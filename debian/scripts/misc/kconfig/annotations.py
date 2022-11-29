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
        self.config = self._parse(raw_data)

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
        config = {}
        for line in data.splitlines():
            m = re.match(r'^# (CONFIG_.*) is not set$', line)
            if m:
                config[m.group(1)] = literal_eval("'n'")
                continue
            m = re.match(r'^(CONFIG_[A-Za-z0-9_]+)=(.*)$', line)
            if m:
                config[m.group(1)] = literal_eval("'" + m.group(2) + "'")
                continue
        return config

class Annotation(Config):
    """
    Parse annotations file, individual config options can be accessed via
    .config[<CONFIG_OPTION>]
    """
    def _parse(self, data: str) -> dict:
        # Parse header
        self.header = ''
        for line in data.splitlines():
            if re.match(r'^#.*', line):
                m = re.match(r'^# ARCH: (.*)', line)
                if m:
                    self.arch = list(m.group(1).split(' '))
                m = re.match(r'^# FLAVOUR: (.*)', line)
                if m:
                    self.flavour = list(m.group(1).split(' '))
                self.header += line + "\n"
            else:
                break

        # Skip comments
        data = re.sub(r'(?m)^\s*#.*\n?', '', data)

        # Handle includes (recursively)
        self.include = []
        expand_data = ''
        for line in data.splitlines():
            m = re.match(r'^include\s+"?([^"]*)"?', line)
            if m:
                self.include.append(m.group(1))
                include_fname = dirname(abspath(self.fname)) + '/' + m.group(1)
                include_data = self._load(include_fname)
                expand_data += include_data + '\n'
            else:
                expand_data += line + '\n'

        # Skip empty, non-policy and non-note lines
        data = "\n".join([l.rstrip() for l in expand_data.splitlines()
             if l.strip() and (re.match('.* policy<', l) or re.match('.* note<', l))])

        # Convert multiple spaces to single space to simplifly parsing
        data = re.sub(r'  *', ' ', data)

        # Parse config/note statements
        config = {}
        for line in data.splitlines():
            try:
                conf = line.split(' ')[0]
                if conf in config:
                    entry = config[conf]
                else:
                    entry = {}
                m = re.match(r'.*policy<(.*)>', line)
                if m:
                    entry['policy'] = literal_eval(m.group(1))
                m = re.match(r'.*note<(.*?)>', line)
                if m:
                    entry['note'] = "'" + m.group(1).replace("'", '') + "'"
                if entry:
                    config[conf] = entry
            except Exception as e:
                raise Exception(str(e) + f', line = {line}')
        return config

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
        for conf in self.config:
            if 'policy' not in self.config[conf]:
                continue
            for flavour in self.flavour:
                if flavour not in self.config[conf]['policy']:
                    continue
                m = re.match(r'^(.*?)-(.*)$', flavour)
                if not m:
                    continue
                arch = m.group(1)
                if arch not in self.config[conf]['policy']:
                    continue
                if self.config[conf]['policy'][flavour] == self.config[conf]['policy'][arch]:
                    del self.config[conf]['policy'][flavour]

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
                if old_val != new_val:
                    if 'policy' in self.config[conf]:
                        val = dict(sorted(self.config[conf]['policy'].items()))
                        line = f"{conf : <47} policy<{val}>"
                        tmp.write(line + "\n")
                    if 'note' in self.config[conf]:
                        val = self.config[conf]['note']
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
