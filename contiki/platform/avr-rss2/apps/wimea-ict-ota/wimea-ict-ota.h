char *trim(char *str);
void error_log(char *message);

void display_node_name(void);
void display_tagmask(void);
void set_default_report_mask(void);
void display_reporting_interval(void);
void check_sensor_connection(void);
void read_sensor_values(void);

void change_node_name(char *name);
void change_reporting_interval(char *value);
void add_to_report_mask(char* parameter);
void remove_from_report_mask(char* parameter);
void auto_set_report_mask(char *value2[], int size1, int sensor_status);
