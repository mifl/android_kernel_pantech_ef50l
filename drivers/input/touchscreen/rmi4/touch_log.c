#include <linux/proc_fs.h>

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_RMI4_PANTECH)
#define TOUCH_E001	1160785969
#define TOUCH_E002	1160785970
#define TOUCH_E003	1160785971
#define TOUCH_E004	1160785972
#define TOUCH_E005	1160785973
#define TOUCH_E006	1160785974
#define TOUCH_E007	1160785975
#define TOUCH_E008	1160785976
#define TOUCH_E009	1160785977
#define TOUCH_E010	1160786224
#define TOUCH_E016	1160786230
#define TOUCH_E017	1160786231
#define TOUCH_E038  1160786744
#define TOUCH_E041  1160786993
#define TOUCH_E042  1160786994
#define TOUCH_E045	1160786997
#define TOUCH_E046	1160786998
#define TOUCH_E047	1160786999
#define TOUCH_E048	1160787000
#define TOUCH_E049	1160787001
#define TOUCH_E050	1160787248
#define TOUCH_E051	1160787249
#define TOUCH_E054	1160787264
#define TOUCH_E055	1160787253
#define TOUCH_E056	1160787254
#define TOUCH_E057	1160787255
#define TOUCH_E058	1160787256
#define TOUCH_E059	1160787257
#define TOUCH_E064	1160787508
#define TOUCH_E068	1160787512
#define TOUCH_E069	1160787513
#define TOUCH_E070	1160787760
#endif

typedef struct {
	int         size;   /* maximum number of elements           */
	int         start;  /* index of oldest element              */
	int         end;    /* index at which to write new element  */
	char *elems;  /* vector of elements                   */
} CircularBuffer;

void cbInit(CircularBuffer *cb, int size) {
	cb->size  = size + 1; /* include empty elem */
	cb->start = 0;
	cb->end   = 0;
	cb->elems = (char*)kmalloc(cb->size * sizeof(char), GFP_KERNEL | GFP_ATOMIC);
}

void cbFree(CircularBuffer *cb) {
	kfree(cb->elems); /* OK if null */ 
}

int cbIsFull(CircularBuffer *cb) {
	return (cb->end + 1) % cb->size == cb->start; 
}

int cbIsEmpty(CircularBuffer *cb) {
	return cb->end == cb->start; 
}

/* Write an element, overwriting oldest element if buffer is full. App can
 *    choose to avoid the overwrite by checking cbIsFull(). */
void cbWrite(CircularBuffer *cb, char *elem) {
	cb->elems[cb->end] = *elem;
	cb->end = (cb->end + 1) % cb->size;
	if (cb->end == cb->start)
		cb->start = (cb->start + 1) % cb->size; /* full, overwrite */
}

/* Read oldest element. App must ensure !cbIsEmpty() first. */
void cbRead(CircularBuffer *cb, char *elem) {
	*elem = cb->elems[cb->start];
	cb->start = (cb->start + 1) % cb->size;
}

CircularBuffer cb;
spinlock_t cb_spinlock;

int read_log(char *page, char **start, off_t off, int count, int *eof, void *data_unused) {
	char *buf;
	char elem = {0};
	buf = page;


	spin_lock(&cb_spinlock);
	while (!cbIsEmpty(&cb)) {
		cbRead(&cb, &elem);
		buf += sprintf(buf, &elem);
	}
	spin_unlock(&cb_spinlock);
	*eof = 1;
	return buf - page;
}
#if defined(CONFIG_TOUCHSCREEN_CYTTSP_GEN4)
char touch_info_vendor[] = "cypress";
char touch_info_chipset[] = "tma768";
#elif defined(CONFIG_TOUCHSCREEN_SYNAPTICS_RMI4_PANTECH)
char touch_info_vendor[] = "synaptics";
char touch_info_chipset[] = "rmi4";
#endif

extern int touch_firmware_version;


int read_touch_info(char *page, char **start, off_t off, int count, int *eof, void *data_unused) {
	char *buf;
	buf = page;

	buf += sprintf(buf, "Vendor: \t%s\n", touch_info_vendor);
	buf += sprintf(buf, "Chipset: \t%s\n", touch_info_chipset);

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_RMI4_PANTECH)
	switch(touch_firmware_version)
	{
		case TOUCH_E001:
			buf += sprintf(buf, "Version: \t e001\n");
			break;
		case TOUCH_E002:
			buf += sprintf(buf, "Version: \t e002\n");
			break;
		case TOUCH_E003:
			buf += sprintf(buf, "Version: \t e003\n");
			break;
		case TOUCH_E004:
			buf += sprintf(buf, "Version: \t e004\n");
			break;
		case TOUCH_E005:
			buf += sprintf(buf, "Version: \t e005\n");
			break;
		case TOUCH_E006:
			buf += sprintf(buf, "Version: \t e006\n");
			break;
		case TOUCH_E007:
			buf += sprintf(buf, "Version: \t e007\n");
			break;
		case TOUCH_E008:
			buf += sprintf(buf, "Version: \t e008\n");
			break;
		case TOUCH_E009:
			buf += sprintf(buf, "Version: \t e009\n");
			break;
		case TOUCH_E010:
			buf += sprintf(buf, "Version: \t e010\n");
			break;
		case TOUCH_E016:
			buf += sprintf(buf, "Version: \t e016\n");
			break;			
		case TOUCH_E017:
			buf += sprintf(buf, "Version: \t e017\n");
			break;
		case TOUCH_E038:
			buf += sprintf(buf, "Version: \t e038\n");
			break;
        case TOUCH_E041:
			buf += sprintf(buf, "Version: \t e041\n");
			break;	
        case TOUCH_E042:
			buf += sprintf(buf, "Version: \t e042\n");
			break;
        case TOUCH_E045:
			buf += sprintf(buf, "Version: \t e045\n");
			break;
        case TOUCH_E046:
			buf += sprintf(buf, "Version: \t e046\n");
			break;    
        case TOUCH_E047:
			buf += sprintf(buf, "Version: \t e047\n");
			break;   
        case TOUCH_E048:
			buf += sprintf(buf, "Version: \t e048\n");
			break;     
        case TOUCH_E049:
			buf += sprintf(buf, "Version: \t e049\n");
			break;            
    	case TOUCH_E050:
			buf += sprintf(buf, "Version: \t e050\n");
			break; 
    	case TOUCH_E051:
			buf += sprintf(buf, "Version: \t e051\n");
			break;
  	    case TOUCH_E054:
			buf += sprintf(buf, "Version: \t e054\n");
			break; 
        case TOUCH_E055:
			buf += sprintf(buf, "Version: \t e055\n");
			break;
        case TOUCH_E056:
			buf += sprintf(buf, "Version: \t e056\n");
			break;  
        case TOUCH_E057:
			buf += sprintf(buf, "Version: \t e057\n");
			break; 
        case TOUCH_E058:
			buf += sprintf(buf, "Version: \t e058\n");
			break;
        case TOUCH_E059:
			buf += sprintf(buf, "Version: \t e059\n");
			break; 
        case TOUCH_E064:
			buf += sprintf(buf, "Version: \t e064\n");
			break;       
        case TOUCH_E068:
			buf += sprintf(buf, "Version: \t e068\n");
			break;   
        case TOUCH_E069:
			buf += sprintf(buf, "Version: \t e069\n");
			break;
        case TOUCH_E070:
			buf += sprintf(buf, "Version: \t e070\n");
			break;      				
		default:
			buf += sprintf(buf, "Version: \t%d\n", touch_firmware_version);
			break;
	}
#endif
	
	*eof = 1;
	return buf - page;
}
char printproc_buf[1024];

void printp(const char *fmt, ...) {
	int count = 0;
	int i;
	va_list args;
	spin_lock(&cb_spinlock);
	va_start(args, fmt);
	count += vsnprintf(printproc_buf, 1024, fmt, args);
	for (i = 0; i<count; i++) {
		cbWrite(&cb, &printproc_buf[i]);
	}
	va_end(args);
	spin_unlock(&cb_spinlock);
}

void init_proc(void) { 
	int testBufferSize = 1024;

	struct proc_dir_entry *touch_log_ent;
	struct proc_dir_entry *touch_info_ent;
	
	touch_log_ent = create_proc_entry("touchlog", S_IFREG|S_IRUGO, 0); 
	touch_log_ent->read_proc = read_log;
	
	touch_info_ent = create_proc_entry("touchinfo", S_IFREG|S_IRUGO, 0); 
	touch_info_ent->read_proc = read_touch_info;

	spin_lock_init(&cb_spinlock);
	cbInit(&cb, testBufferSize);
}

void remove_proc(void) {
	remove_proc_entry("touchlog", 0);
	remove_proc_entry("touchinfo", 0);
	cbFree(&cb);
}
