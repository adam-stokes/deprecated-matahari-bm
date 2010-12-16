enum Prop {
    PROP_0,
    PROP_UUID,
    PROP_HOSTNAME,
    PROP_IS_VIRTUAL,
    PROP_OPERATING_SYSTEM,
    PROP_MEMORY,
    PROP_SWAP,
    PROP_ARCH,
    PROP_PLATFORM,
    PROP_PROCESSORS,
    PROP_CORES,
    PROP_MODEL,
    PROP_LAST_UPDATED_SEQ,
    PROP_LAST_UPDATED,
    PROP_LOAD_AVERAGE_1,
    PROP_LOAD_AVERAGE_5,
    PROP_LOAD_AVERAGE_15,
    PROP_MEM_FREE,
    PROP_SWAP_FREE,
    PROP_PROC_TOTAL,
    PROP_PROC_RUNNING,
    PROP_PROC_SLEEPING,
    PROP_PROC_ZOMBIE,
    PROP_PROC_STOPPED,
    PROP_PROC_IDLE,
    };


typedef struct {
    enum Prop prop;
    gchar *name, *nick, *desc;
    GParamFlags flags;
    char type;
} Property;

Property properties_Host[] = {
    { PROP_UUID, "uuid", "uuid", "Host UUID", G_PARAM_READABLE, 's' },
    { PROP_HOSTNAME, "hostname", "hostname", "Hostname", G_PARAM_READABLE, 's' },
    { PROP_IS_VIRTUAL, "is_virtual", "is_virtual", "Is this machine virtual?", G_PARAM_READABLE, 'b' },
    { PROP_OPERATING_SYSTEM, "operating_system", "operating_system", "The installed operating system.", G_PARAM_READABLE, 's' },
    { PROP_MEMORY, "memory", "memory", "Amount of primary memory for host (kb)", G_PARAM_READABLE, 't' },
    { PROP_SWAP, "swap", "swap", "Amount of swap for host (kb)", G_PARAM_READABLE, 't' },
    { PROP_ARCH, "arch", "arch", "Architecture of host", G_PARAM_READABLE, 's' },
    { PROP_PLATFORM, "platform", "platform", "The wordsize for the host.", G_PARAM_READABLE, 'y' },
    { PROP_PROCESSORS, "processors", "processors", "The number of physical CPUs.", G_PARAM_READABLE, 'y' },
    { PROP_CORES, "cores", "cores", "The total number of processor cores.", G_PARAM_READABLE, 'y' },
    { PROP_MODEL, "model", "model", "The processor(s) model description.", G_PARAM_READABLE, 's' },
    { PROP_LAST_UPDATED_SEQ, "last_updated_seq", "last_updated_seq", "The heartbeat sequence number.", G_PARAM_READABLE, 'u' },
    { PROP_LAST_UPDATED, "last_updated", "last_updated", "The last time a heartbeat occurred.", G_PARAM_READABLE, 'i' },
    { PROP_LOAD_AVERAGE_1, "load_average_1", "load_average_1", "The one minute load average.", G_PARAM_READABLE, 'd' },
    { PROP_LOAD_AVERAGE_5, "load_average_5", "load_average_5", "The five minute load average.", G_PARAM_READABLE, 'd' },
    { PROP_LOAD_AVERAGE_15, "load_average_15", "load_average_15", "The fiften minute load average", G_PARAM_READABLE, 'd' },
    { PROP_MEM_FREE, "mem_free", "mem_free", "Amount of available memory for host (kb)", G_PARAM_READABLE, 't' },
    { PROP_SWAP_FREE, "swap_free", "swap_free", "Amount of available swap for host (kb)", G_PARAM_READABLE, 't' },
    { PROP_PROC_TOTAL, "proc_total", "proc_total", "Total processes", G_PARAM_READABLE, 't' },
    { PROP_PROC_RUNNING, "proc_running", "proc_running", "Total running processes", G_PARAM_READABLE, 't' },
    { PROP_PROC_SLEEPING, "proc_sleeping", "proc_sleeping", "Total sleeping processes", G_PARAM_READABLE, 't' },
    { PROP_PROC_ZOMBIE, "proc_zombie", "proc_zombie", "Total zombie processes", G_PARAM_READABLE, 't' },
    { PROP_PROC_STOPPED, "proc_stopped", "proc_stopped", "Total stopped processes", G_PARAM_READABLE, 't' },
    { PROP_PROC_IDLE, "proc_idle", "proc_idle", "Total idle processes", G_PARAM_READABLE, 't' },
    { PROP_0, NULL, NULL, NULL, 0, '\0' },
};