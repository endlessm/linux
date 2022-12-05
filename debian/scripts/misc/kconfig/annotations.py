#!/usr/bin/env python
# -*- mode: python -*-
# python module to manage Ubuntu kernel .config and annotations
# Copyright © 2022 Canonical Ltd.

import json
import re
import shutil
import tempfile
from ast import literal_eval
from os.path import dirname, abspath

class Config(object):
    def __init__(self, fname: str, arch: str = None, flavour: str = None):
        """
        Basic configuration file object
        """
        self.fname = fname
        raw_data = self._load(fname)
        self._parse(raw_data)

    def _load(self, fname: str) -> str:
        with open(fname, 'rt') as fd:
            data = fd.read()
        return data.rstrip()

    def __str__(self):
        """ Return a JSON representation of the config """
        return json.dumps(self.config, indent=4)

class KConfig(Config):
    """
    Parse a .config file, individual config options can be accessed via
    .config[<CONFIG_OPTION>]
    """
    def _parse(self, data: str) -> dict:
        self.config = {}
        for line in data.splitlines():
            m = re.match(r'^# (CONFIG_.*) is not set$', line)
            if m:
                self.config[m.group(1)] = literal_eval("'n'")
                continue
            m = re.match(r'^(CONFIG_[A-Za-z0-9_]+)=(.*)$', line)
            if m:
                self.config[m.group(1)] = literal_eval("'" + m.group(2) + "'")
                continue

class Annotation(Config):
    """
    Parse body of annotations file
    """
    def _parse_body(self, data: str):
        # Skip comments
        data = re.sub(r'(?m)^\s*#.*\n?', '', data)

        # Convert multiple spaces to single space to simplifly parsing
        data = re.sub(r'  *', ' ', data)

        # Handle includes (recursively)
        for line in data.splitlines():
            m = re.match(r'^include\s+"?([^"]*)"?', line)
            if m:
                self.include.append(m.group(1))
                include_fname = dirname(abspath(self.fname)) + '/' + m.group(1)
                include_data = self._load(include_fname)
                self._parse_body(include_data)
            else:
                # Skip empty, non-policy and non-note lines
                if re.match('.* policy<', line) or re.match('.* note<', line):
                    try:
                        # Parse single policy or note rule
                        conf = line.split(' ')[0]
                        if conf in self.config:
                            entry = self.config[conf]
                        else:
                            entry = {'policy': {}}
                        m = re.match(r'.*policy<(.*)>', line)
                        if m:
                            entry['policy'] |= literal_eval(m.group(1))
                        else:
                            m = re.match(r'.*note<(.*?)>', line)
                            if m:
                                entry['note'] = "'" + m.group(1).replace("'", '') + "'"
                            else:
                                raise Exception('syntax error')
                        self.config[conf] = entry
                    except Exception as e:
                        raise Exception(str(e) + f', line = {line}')

    """
    Parse main annotations file, individual config options can be accessed via
    self.config[<CONFIG_OPTION>]
    """
    def _parse(self, data: str) -> dict:
        self.config = {}
        self.arch = []
        self.flavour = []
        self.flavour_dep = {}
        self.include = []
        self.header = ''

        # Parse header (only main header will considered, headers in includes
        # will be treated as comments)
        for line in data.splitlines():
            if re.match(r'^#.*', line):
                m = re.match(r'^# ARCH: (.*)', line)
                if m:
                    self.arch = list(m.group(1).split(' '))
                m = re.match(r'^# FLAVOUR: (.*)', line)
                if m:
                    self.flavour = list(m.group(1).split(' '))
                m = re.match(r'^# FLAVOUR_DEP: (.*)', line)
                if m:
                    self.flavour_dep = eval(m.group(1))
                self.header += line + "\n"
            else:
                break

        # Parse body (handle includes recursively)
        self._parse_body(data)

    def _remove_entry(self, config : str):
        if 'policy' in self.config[config]:
            del self.config[config]['policy']
        if 'note' in self.config[config]:
            del self.config[config]['note']
        if not self.config[config]:
            del self.config[config]

    def remove(self, config : str, arch: str = None, flavour: str = None):
        if config not in self.config:
            return
        if arch is not None:
            if flavour is not None:
                flavour = f'{arch}-{flavour}'
            else:
                flavour = arch
            del self.config[config]['policy'][flavour]
            if not self.config[config]['policy']:
                self._remove_entry(config)
        else:
            self._remove_entry(config)

    def set(self, config : str, arch: str = None, flavour: str = None,
            value : str = None, note : str = None):
        if value is not None:
            if config not in self.config:
                self.config[config] = { 'policy': {} }
            if arch is not None:
                if flavour is not None:
                    flavour = f'{arch}-{flavour}'
                else:
                    flavour = arch
                self.config[config]['policy'][flavour] = value
            else:
                for arch in self.arch:
                    self.config[config]['policy'][arch] = value
        if note is not None:
            self.config[config]['note'] = "'" + note.replace("'", '') + "'"

    def update(self, c: KConfig, arch: str, flavour: str = None, configs: list = []):
        """ Merge configs from a Kconfig object into Annotation object """

        # Determine if we need to import all configs or a single config
        if not configs:
            configs = c.config.keys()
            configs |= self.search_config(arch=arch, flavour=flavour).keys()

        # Import configs from the Kconfig object into Annotations
        if flavour is not None:
            flavour = arch + f'-{flavour}'
        else:
            flavour = arch
        for conf in configs:
            if conf in c.config:
                val = c.config[conf]
            else:
                val = '-'
            if conf in self.config:
                if 'policy' in self.config[conf]:
                    self.config[conf]['policy'][flavour] = val
                else:
                    self.config[conf]['policy'] = {flavour: val}
            else:
                self.config[conf] = {'policy': {flavour: val}}

    def _compact(self):
        # Try to remove redundant settings: if the config value of a flavour is
        # the same as the one of the main arch simply drop it.
        for conf in self.config.copy():
            if 'policy' not in self.config[conf]:
                continue
            for flavour in self.flavour:
                if flavour not in self.config[conf]['policy']:
                    continue
                m = re.match(r'^(.*?)-(.*)$', flavour)
                if not m:
                    continue
                arch = m.group(1)
                if arch in self.config[conf]['policy']:
                    if self.config[conf]['policy'][flavour] == self.config[conf]['policy'][arch]:
                        del self.config[conf]['policy'][flavour]
                        continue
                if flavour not in self.flavour_dep:
                    continue
                generic = self.flavour_dep[flavour]
                if generic in self.config[conf]['policy']:
                    if self.config[conf]['policy'][flavour] == self.config[conf]['policy'][generic]:
                        del self.config[conf]['policy'][flavour]
                        continue
            for flavour in self.config[conf]['policy'].copy():
                if flavour not in list(set(self.arch + self.flavour)):
                    del self.config[conf]['policy'][flavour]
            if not self.config[conf]['policy']:
                del self.config[conf]

    def save(self, fname: str):
        """ Save annotations data to the annotation file """
        # Compact annotations structure
        self._compact()

        # Save annotations to disk
        with tempfile.NamedTemporaryFile(mode='w+t', delete=False) as tmp:
            # Write header
            tmp.write(self.header + '\n')

            # Write includes
            for i in self.include:
                tmp.write(f'include "{i}"\n')
            if self.include:
                tmp.write("\n")

            # Write config annotations and notes
            tmp.flush()
            shutil.copy(tmp.name, fname)
            tmp_a = Annotation(fname)

            # Only save local differences (preserve includes)
            for conf in sorted(self.config):
                old_val = tmp_a.config[conf] if conf in tmp_a.config else None
                new_val = self.config[conf]
                # If new_val is a subset of old_val, skip it
                if old_val and 'policy' in old_val and 'policy' in new_val:
                    if old_val['policy'] == old_val['policy'] | new_val['policy']:
                        continue
                if 'policy' in new_val:
                    val = dict(sorted(new_val['policy'].items()))
                    line = f"{conf : <47} policy<{val}>"
                    tmp.write(line + "\n")
                    if 'note' in new_val:
                        val = new_val['note']
                        line = f"{conf : <47} note<{val}>"
                        tmp.write(line + "\n\n")

            # Replace annotations with the updated version
            tmp.flush()
            shutil.move(tmp.name, fname)

    def search_config(self, config: str = None, arch: str = None, flavour: str = None) -> dict:
        """ Return config value of a specific config option or architecture """
        if flavour is None:
            flavour = 'generic'
        flavour = f'{arch}-{flavour}'
        if flavour in self.flavour_dep:
            generic = self.flavour_dep[flavour]
        else:
            generic = flavour
        if config is None and arch is None:
            # Get all config options for all architectures
            return self.config
        elif config is None and arch is not None:
            # Get config options of a specific architecture
            ret = {}
            for c in self.config:
                if not 'policy' in self.config[c]:
                    continue
                if flavour in self.config[c]['policy']:
                    ret[c] = self.config[c]['policy'][flavour]
                elif generic != flavour and generic in self.config[c]['policy']:
                    ret[c] = self.config[c]['policy'][generic]
                elif arch in self.config[c]['policy']:
                    ret[c] = self.config[c]['policy'][arch]
            return ret
        elif config is not None and arch is None:
            # Get a specific config option for all architectures
            return self.config[config] if config in self.config else None
        elif config is not None and arch is not None:
            # Get a specific config option for a specific architecture
            if config in self.config:
                if 'policy' in self.config[config]:
                    if flavour in self.config[config]['policy']:
                        return {config: self.config[config]['policy'][flavour]}
                    elif generic != flavour and generic in self.config[config]['policy']:
                        return {config: self.config[config]['policy'][generic]}
                    elif arch in self.config[config]['policy']:
                        return {config: self.config[config]['policy'][arch]}
        return None

    @staticmethod
    def to_config(data: dict) -> str:
        """ Convert annotations data to .config format """
        s = ''
        for c in data:
            v = data[c]
            if v == 'n':
                s += f"# {c} is not set\n"
            elif v == '-':
                pass
            else:
                s += f"{c}={v}\n"
        return s.rstrip()
