

enum CHARGER_MODE {
	CHARGER_OFF = 0,
    CHARGER_ON = 1,
    CALL_ON = 3,
    CALL_OFF = 4
};
void touch_fops_init(void);
void touch_fops_exit(void);

int send_reference_data(unsigned long arg);
int send_reference_data_for_self(unsigned long arg);
int panel_test(int count);
void rmi_chargermode(int data);
int rmi_scan_function(void);
int rmi_raw_capacitance(void);
int rmi_baseline(int limit);
int rmi_delta(void);



