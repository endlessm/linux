Notes regaring API Usage                {#apiusage}
========================

There are some restrictions on the
[Application Interface](@ref ApplicationInterface) with respect to the state of
the master instance and the calling context, which are explained in the
following.

## Rules of Thumb

All configuration (`ecrt_slave_config_*()`) has to be done in Linux process
context. They can be blocking, so take care when holding locks. After
ecrt_master_activate() ing the master, your application must not alter the
slave configuration. Instead, update process data using ecrt_domain_queue()
and ecrt_domain_process() or use the asynchronous interface like
ecrt_sdo_request_read(). Don't forget to ecrt_master_receive() and
ecrt_master_send(). These functions can be called from non-process context
too, like Xenomai/RTAI applications or custom kernel modules.

## Master Phase

The first distinction of cases is whether ecrt_master_activate() has been
called or not. Before ecrt_master_activate() (or after
ecrt_master_deactivate()), the master is in idle phase. Sending and receiving
EtherCAT frames will be done by the master itself, the application (e. g. you)
can store slave configurations for later use. After ecrt_master_activate(),
the master switches into operation mode. The application is now in charge of
steering the communication. Process data can be exchanged under real time
constraints.  Altering the slave configuration is not possible anymore.

| Tag           | Description                                                                           |
|---------------|---------------------------------------------------------------------------------------|
| `master_op`   | Master must be in operation phase, so after `ecrt_master_activate()` has been called. |
| `master_idle` | Master must be in idle phase, so before `ecrt_master_activate()` has been called.     |
| `master_any`  | Master can be in idle or operation phase.                                             |

## Allowed Context

The second distinction of cases is the calling context, which means how the
application is run. Most of the functions of the
[Application Interface](@ref ApplicationInterface) have to acquire locks or
allocate memory, so they are potentially sleeping. They are tagged as
`blocking`. Sleeping is not allowed in all contexts, for instance when using
Xenomai/RTAI or a kernel timer. Only a very limited set of functions can be
called from any context, marked as `rt_safe`. They do not allocate memory and
will not block.

| Tag        | Description                                                                                   |
|------------|-----------------------------------------------------------------------------------------------|
| `rt_safe`  | Realtime context (RT userspace, atomic/softirq context in kernel, Xenomai/RTAI RT task) safe. |
| `blocking` | Linux process context only (userspace or kernel), might block.                                |
