struct svc_action_private_s 
{
	char *exec;
	char *args[4];
	gboolean cancel;
	
	guint repeat_timer;
	void (*callback)(svc_action_t *op);
	
	int            stderr_fd;
	mainloop_fd_t *stderr_gsource;

	int            stdout_fd;
	mainloop_fd_t *stdout_gsource;
};

extern GList *services_os_get_directory_list(const char *root, gboolean files);
extern gboolean services_os_action_execute(svc_action_t* op, gboolean synchronous);
