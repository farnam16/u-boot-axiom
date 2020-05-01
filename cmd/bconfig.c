/*
 * Author: Davide Cardillo <davide.cardillo@seco.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 *
 * This code provide a tool to properly configure the boot environment
 * for the AXIOM Boards.
 *
 */


#include <common.h>
#include <command.h>
#include <environment.h> 
#include <stdio_dev.h>

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <stdio_dev.h>
#include <timestamp.h>
#include <version.h>
#include <net.h>
#include <serial.h>
#include <nand.h>
#include <onenand_uboot.h>
#include <mmc.h>
#include <watchdog.h>



/*  __________________________________________________________________________
 * |                                                                          |
 * |                                   IMAGE                                  |
 * |__________________________________________________________________________|
 */
#define ENV_IMAGE_SRC_MMC                   "run emmc_boot"

#define ENV_IMAGE_SRC_USD                   "run sd_boot"

#define ENV_IMAGE_SRC_USB                   "run usb_boot"

#define ENV_IMAGE_SRC_SPI                   "run spi_boot"

#define ENV_IMAGE_SRC_TFTP                  "run tftp_boot"


#define GET_IMAGE_PATH                      getenv ("image_name")

#define SAVE_IMAGE_DEVICE_ID(x)             setenv ("image_device_id", (x))
#define SAVE_IMAGE_PARTITION(x)             setenv ("image_partition", (x))
#define SAVE_IMAGE_LOADADDR(x)              setenv ("image_loadaddr", (x))
#define SAVE_IMAGE_PATH(x)                  setenv ("image_name", (x))
/* da rivedere */
#define SAVE_IMAGE_SPI_ADDR(x)              setenv ("kernel_spi_addr", (x))
#define SAVE_IMAGE_SPI_LEN(x)               setenv ("kernel_spi_len", (x))
#define GET_ENV_IMAGE_SRC(srv)              ( ENV_KERNEL_SRC_ ## (src) )
#define SAVE_IMAGE_BOOT_LOAD(x)             setenv ("image_load", (x))


/*  __________________________________________________________________________
 * |                                                                          |
 * |                                 FILE SYSTEM                              |
 * |__________________________________________________________________________|
 */
#define ENV_FS_SRC_MMC                      "run emmc_root_load"

#define ENV_FS_SRC_USD                      "run sd_root_load"

#define ENV_FS_SRC_USB                      "run usb_root_load"

#define ENV_FS_SRC_NFS                      "run nfs_root_load"


#define SAVE_FILESYSTEM_DEVICE_ID(x)        setenv ("root_device_id", (x))
#define SAVE_FILESYSTEM_PARTITION(x)        setenv ("root_partition", (x))
#define SAVE_FILESYSTEM_ROOT(x)             setenv ("root_load", (x))
#define GET_ENV_FS_SRC(srv)                 ( ENV_FS_SRC_ ## src )



/*  __________________________________________________________________________
 * |                                                                          |
 * |                               BOOTADRGS BASE                             |
 * |__________________________________________________________________________|
 */

#define DEF_TFTP_SERVER_IP      "10.0.0.1"
#define DEF_TFTP_CLIENT_IP      "10.0.0.100"

#define CONSOLE_INTERFACE   \
	"console="CONFIG_CONSOLE_DEV "," __stringify(CONFIG_BAUDRATE) "\0"

#define BOOTARGS_BASE       \
	"setenv bootargs ${console_interface} ${memory} ${cpu_freq} ${videomode}"



/*  __________________________________________________________________________
 * |                                                                          |
 * |                               NFS PARAMETERS                             |
 * |__________________________________________________________________________|
 */

#define DEF_IP_LOCAL        "10.0.0.100"
#define DEF_IP_SERVER       "10.0.0.1"
#define DEF_NETMASK         "255.255.255.0"
#define DEF_NFS_PATH        "/targetfs/"

#define GET_IP_LOCAL        getenv ("nfs_ip_client")
#define GET_IP_SERVER       getenv ("nfs_ip_server")
#define GET_NETMASK         getenv ("nfs_netmask")
#define GET_NFS_PATH        getenv ("nfs_path")

#define SAVE_NFS_USE(x)          setenv ("run_from_nfs", (x))
#define SAVE_NFS_IP_CLIENT(x)    setenv ("nfs_ip_client", (x))
#define SAVE_NFS_IP_SERVER(x)    setenv ("nfs_ip_server", (x))
#define SAVE_NFS_NETMASK(x)      setenv ("nfs_netmask", (x))
#define SAVE_NFS_PATH(x)         setenv ("nfs_path", (x))
#define SAVE_NFS_USE_DHCP(x)     setenv ("nfs_use_dhcp", (x))


#define SET_IPCONF_NO_DHCP  \
	"setenv ip \"${nfs_ip_client}:::${nfs_netmask}::eth0:off\""

#define SET_IPCONF_DHCP     \
	"setenv ip \":::::eth0:dhcp\""

#define SET_IP              \
	"if test \"${nfs_use_dhcp}\" = \"0\"; then run set_ipconf_no_dhcp; else run set_ipconf_dhcp; fi"

#define SET_NFSROOT         \
	"setenv nfsroot \"${nfs_ip_server}:${nfs_path}\""



/*  __________________________________________________________________________
 * |                                                                          |
 * |                                  COMMAND                                 |
 * |__________________________________________________________________________|
 */
#define SET_BOOT_DEV       \
	"run set_kernel_boot_dev; run set_fdt_boot_dev; run set_root_dev;"

#define LOAD_BOOT_DEV      \
	"run kernel_boot_dev; run fdt_boot_dev;"

#define LOAD_ROOT_DEV      \
	"setenv bootargs ${bootargs} ${root_dev}"

#define CMD_BOOT           \
	"run bootargs_base; run set_boot_dev; run load_boot_dev; run load_root_dev; " \
	"bootz ${kernel_loadaddr} - ${fdt_loadaddr}"



/*  __________________________________________________________________________
 * |                                                                          |
 * |                            BOARD SPECIFICATION                           |
 * |__________________________________________________________________________|
 */
typedef enum {
	DEV_EMMC,
	DEV_U_SD,
	DEV_NAND,
	DEV_SPI,
	DEV_SATA,
	DEV_USB,
	DEV_TFTP,
	DEV_NFS,
} device_t;


typedef struct data_boot_dev {
	device_t         dev_type;
	char             *label;
	char             *env_str;
	char             *device_id;
	char             *load_address;
	char             *def_path;
} data_boot_dev_t;


#define BOOT_DEV_ID_EMMC                    __stringify(CONFIG_BOOT_ID_EMMC)"\0" 
#define BOOT_DEV_ID_U_SD                    __stringify(CONFIG_BOOT_ID_USD)"\0" 
#define BOOT_DEV_ID_SPI                     "0"
#define BOOT_DEV_ID_USB0                    "0"
#define BOOT_DEV_ID_USB1                    "1"

#define ROOT_DEV_ID_EMMC                    __stringify(CONFIG_ROOT_ID_EMMC)"\0"
#define ROOT_DEV_ID_U_SD                    __stringify(CONFIG_ROOT_ID_USD)"\0"
#define ROOT_DEV_ID_EXT_SD                  __stringify(CONFIG_ROOT_ID_EXT_SD)"\0"

#define LOAD_ADDR_IMAGE_LOCAL_DEV           __stringify(IMAGE_LOADADDR)"\0"
#define LOAD_ADDR_IMAGE_REMOTE_DEV          __stringify(IMAGE_LOADADDR)"\0"
/* da rivedere */
#define SPI_LOAD_ADDRESS                    __stringify(CONFIG_SPI_IMAGE_LOADADDR)"\0"
#define SPI_LOAD_LEN                        __stringify(CONFIG_SPI_IMAEG_LEN)"\0"


static data_boot_dev_t image_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_IMAGE_SRC_MMC,    BOOT_DEV_ID_EMMC,    LOAD_ADDR_IMAGE_LOCAL_DEV,   "image.ub" },
	{ DEV_U_SD,     "uSD onboard",    ENV_IMAGE_SRC_USD,    BOOT_DEV_ID_U_SD,    LOAD_ADDR_IMAGE_LOCAL_DEV,   "image.ub" },
	{ DEV_SPI,      "SPI onboard",    ENV_IMAGE_SRC_SPI,    BOOT_DEV_ID_SPI,     LOAD_ADDR_IMAGE_LOCAL_DEV,   "image.ub" },
	{ DEV_TFTP,     "TFTP",           ENV_IMAGE_SRC_TFTP,   "",                  LOAD_ADDR_IMAGE_REMOTE_DEV,  "image.ub" },
	{ DEV_USB,      "USB0",           ENV_IMAGE_SRC_USB,    BOOT_DEV_ID_USB0,    LOAD_ADDR_IMAGE_LOCAL_DEV,   "image.ub" },
	{ DEV_USB,      "USB1",           ENV_IMAGE_SRC_USB,    BOOT_DEV_ID_USB0,    LOAD_ADDR_IMAGE_LOCAL_DEV,   "image.ub" },
};


static data_boot_dev_t filesystem_dev_list [] = {
	{ DEV_EMMC,     "eMMC onboard",   ENV_FS_SRC_MMC,     ROOT_DEV_ID_EMMC,    "" },
	{ DEV_U_SD,     "external SD",    ENV_FS_SRC_USD,     ROOT_DEV_ID_U_SD,    "" },
	{ DEV_NFS,      "NFS",            ENV_FS_SRC_NFS,     "",                  "" },
	{ DEV_USB,      "USB",            ENV_FS_SRC_USB,     "",                  "" },
};


/*  __________________________________________________________________________
 * |                                                                          |
 * |                             common functions                             |
 * |__________________________________________________________________________|
 */
int atoi (char *string) {
	int length;
	int retval = 0;
	int i;
	int sign = 1;

	length = strlen(string);
	for (i = 0; i < length; i++) {
		if (0 == i && string[0] == '-') {
			sign = -1;
			continue;
		}
		if (string[i] > '9' || string[i] < '0') {
			break;
		}
		retval *= 10;
		retval += string[i] - '0';
	}
	retval *= sign;
	return retval;
}


int ctoi (char ch) {
	int retval = 0;
	if (ch <= '9' && ch >= '0') {
		retval = ch - '0';
	}
	return retval;
}


static char *getline (void) {
	static char buffer[100];
	char c;
	size_t i;

	i = 0;
	while (1) {
		buffer[i] = '\0';
		while (!tstc()){
			WATCHDOG_RESET();
			continue;
		}

		c = getc();
		/* Convert to uppercase */
		//if (c >= 'a' && c <= 'z')
		//	c -= ('a' - 'A');

		switch (c) {
			case '\r':	/* Enter/Return key */
			case '\n':
				puts("\n");
				return buffer;

			case 0x03:	/* ^C - break */
				return NULL;

			case 0x08:	/* ^H  - backspace */
			case 0x7F:	/* DEL - backspace */
				if (i) {
					puts("\b \b");
					i--;
				}
				break;

			default:
				/* Ignore control characters */
				if (c < 0x20)
					break;
				/* Queue up all other characters */
				buffer[i++] = c;
				printf("%c", c);
				break;
		}
	}
}




/*  __________________________________________________________________________
 * |                                                                          |
 * |                  image.ub/FileSystem Source Selection                    |
 * |__________________________________________________________________________|
 */
#define MIN_PARTITION_ID 1
#define MAX_PARTITION_ID 9

int selection_dev (char *scope, data_boot_dev_t dev_list[], int num_element) {
	char ch;
	int i, selection = 0;

	do {
		printf ("\n __________________________________________________");
		printf ("\n            Chose boot Device for %s.\n", scope);
		printf (" __________________________________________________\n");
		for ( i = 0 ; i < num_element ; i++ ) {
			printf ("%d) %s\n", i+1, dev_list[i].label);
		}
		printf ("> ");
		ch = getc ();
		printf ("%c\n", ch);
	} while ((ctoi(ch) < 1) || (ctoi(ch) > num_element));
	
	selection = ctoi(ch) - 1;

	return selection;
}


void select_partition_id (char *partition_id) {
	char ch;

	do {
		printf ("Chose the partition\n");
		printf ("> ");
		ch = getc ();
		printf ("%c\n", ch);
	} while (ctoi(ch) < MIN_PARTITION_ID || ctoi(ch) > MAX_PARTITION_ID);

	sprintf (partition_id, "%c", ch);
}


void select_source_path (char *source_path, char *subject, char *default_path) {
	char *line;

	printf ("Path of the %s (enter for default %s) > ", subject, default_path);
	line = getline ();

	strcpy (source_path, strcmp (line, "") == 0 ? default_path : line);
}


void select_spi_parameters (char *spi_load_addr, char *spi_load_len) {
	char *line;

	printf ("Load addres of the SPI device (enter for default %s) > ", SPI_LOAD_ADDRESS);
        line = getline ();
	*spi_load_addr = strcmp (line, "") == 0 ? (ulong)SPI_LOAD_ADDRESS : (ulong)atoi(line);

	printf ("Size of data to read (enter for default %s) > ", SPI_LOAD_LEN);
        line = getline ();
	*spi_load_len = strcmp (line, "") == 0 ? (ulong)SPI_LOAD_LEN : (ulong)atoi(line);

	free (line);
}


void select_tftp_parameters (char *serverip_tftp , char *ipaddr_tftp) {
	char *line;
	static char str[20];
	char *pstr;
	
	printf ("\n ________________________________________________________\n");
	printf (" Select TFTP as source for kernel/FDT, please set the net\n");
	printf (" ________________________________________________________\n");

	do {
		pstr = getenv ("serverip");
		if ( pstr == NULL ) {
			strcpy (str, DEF_TFTP_SERVER_IP);
			pstr = &str[0];
		}
		printf ("\nInsert the address ip of the tftp server (enter for current: %s)\n", 
				pstr);
		printf ("> ");
                line = getline ();
                printf ("%s\n", line);
        } while (0);
	strcpy (serverip_tftp, strcmp (line, "") == 0 ? pstr : line);

	do {
		pstr = getenv ("ipaddr");
		if ( pstr == NULL ) {
			strcpy (str, DEF_TFTP_CLIENT_IP);
			pstr = &str[0];
		}
		printf ("\nInsert the address ip of this tftp client (enter for current: %s)\n",
				pstr);
		printf ("> ");
                line = getline ();
                printf ("%s\n", line);
        } while (0);
	strcpy (ipaddr_tftp, strcmp (line, "") == 0 ? pstr : line);
}


int select_image_source (char *partition_id, char *file_name,
			char *spi_load_addr, char *spi_load_len, int *use_tftp) {

	char *str;
	int dev_selected = selection_dev ("Kernel", image_dev_list, ARRAY_SIZE(image_dev_list));

	switch ( image_dev_list[dev_selected].dev_type ) {
		case DEV_EMMC:
		case DEV_U_SD:
		case DEV_SATA:
		case DEV_USB:
			select_partition_id (partition_id);
			break;
		case DEV_SPI:
			select_spi_parameters (spi_load_addr, spi_load_len);
			break;
		case DEV_TFTP:
			*use_tftp = 1;
			break;
		default:
			break;
	}

	str = GET_IMAGE_PATH;
	if ( str != NULL )
		select_source_path (file_name, "Kernel", str);
	else
		select_source_path (file_name, "Kernel", image_dev_list[dev_selected].def_path);

	return dev_selected;
}


void select_nfs_parameters (char *ip_local, char *ip_server, char *nfs_path, char *netmask, int *dhcp_set) {
	char *line;
	char ch;
	static char str[30];
	char *pstr;


	/*  Set DHCP configuration  */
	do { 
		printf ("\nDo you want to use dynamic ip assignment (DHCP)? (y/n)\n");
		printf ("> ");
		ch = getc ();
	} while (ch != 'y' && ch != 'n');
	if (ch == 'y') {
		*dhcp_set = 1;
		printf ("\nYou have select to use dynamic ip\n");
	} else {
		*dhcp_set = 0;
		printf ("\nYou have select to use static ip\n");
	}


	/*  Set HOST IP  */
	do {
		pstr = GET_IP_SERVER;
		if ( pstr == NULL ) {
			strcpy (str, DEF_IP_SERVER);
			pstr = &str[0];
		}

		printf ("Insert the address ip of the host machine (enter for current: %s)\n",
				pstr);
		printf ("> ");
		line = getline ();
		printf ("%s\n", line);
	} while (0);
	strcpy (ip_server, strcmp (line, "") == 0 ? pstr : line);


	/* Set the NFS Path  */
	do {
		pstr = GET_NFS_PATH;
		if ( pstr == NULL ) {
			strcpy (str, DEF_NFS_PATH);
			pstr = &str[0];
		}

		printf ("Insert the nfs path of the host machine (enter for current: %s)\n",
				pstr);
		printf ("> ");
		line = getline ();
		printf ("%s\n", line);
	} while (0);
	strcpy (nfs_path, strcmp (line, "") == 0 ? pstr : line);


	if (*dhcp_set == 0) {
		/* Set CLIENT IP  */
		do {
			pstr = GET_IP_LOCAL;
			if ( pstr == NULL ) {
				strcpy (str, DEF_IP_LOCAL);
				pstr = &str[0];
			}

			printf ("Insert an address ip for this board (enter for current: %s)\n",
					 pstr);
			printf ("> ");
			line = getline ();
			printf ("%s\n", line);
		} while (0);
		strcpy (ip_local, strcmp (line, "") == 0 ? pstr : line);


		/* set NETMASK of the CLIENT  */
		do {
			pstr = GET_NETMASK;
			if ( pstr == NULL ) {
				strcpy (str, DEF_NETMASK);
				pstr = &str[0];
			}

			printf ("Insert the netmask (enter for current: %s)\n",
					pstr);
			printf ("> ");
			line = getline ();
			printf ("%s\n", line);
		} while (0);
		strcpy (netmask, strcmp (line, "") == 0 ? pstr : line);
	}

}

int select_filesystem_souce (char *partition_id, char *nfs_path, 
			char *serverip_nfs , char *ipaddr_nfs, char *netmask_nfs, int *use_dhcp) {

	int dev_selected = selection_dev ("FileSystem", filesystem_dev_list, ARRAY_SIZE(filesystem_dev_list));

	switch ( filesystem_dev_list[dev_selected].dev_type ) {
		case DEV_EMMC:
		case DEV_U_SD:
		case DEV_SATA:
		case DEV_USB:
			select_partition_id (partition_id);
			break;
		case DEV_NFS:
			select_nfs_parameters (ipaddr_nfs, serverip_nfs, nfs_path, netmask_nfs, use_dhcp);
			break;
		default:
			break;
	}

	return dev_selected;
}






static int do_board_config (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	/* generic  */
	int    use_tftp = 0;
	char   serverip_tftp[25];
	char   ipaddr_tftp[25];

	int    set_image = 0;
	int    set_fs    = 0;

	/*  for image.ub  */
	int    image_selected_device;
	char   image_part_id[2];
	char   image_filename[100];
	char   image_spi_load_address[20];
	char   image_spi_load_length[20];

	/*  for filesystem  */
	int    filesystem_selected_device;
	char   filesystem_part_id[2];
	char   filesystem_server_nfs[25];
	char   filesystem_ipaddr_nfs[25];
	char   filesystem_netmask_nfs[25];
	int    filesystem_use_dhcp;
	char   filesystem_path_nfs[100];
	char   filesystem_boot_string[300];

	
	if (argc > 2)
		return cmd_usage (cmdtp);


	if (argc == 2 && strcmp(argv[1], "help") == 0)
		return cmd_usage (cmdtp);

	
	if (argc == 2) {

		if (strcmp(argv[1], "default") == 0) {
			set_default_env ("## Resetting to default environment");
		}

		if (strcmp(argv[1], "image") == 0) {
			set_image = 1;
		}

		if (strcmp(argv[1], "fs") == 0) {
			set_fs = 1;
		}
	}


	if (argc == 1) {
		set_image = 1;
		set_fs    = 1;
	}

/*  __________________________________________________________________________
 * |_______________________________ IMAGE.UB _________________________________|
 */
	if ( set_image ) { 

		image_selected_device = select_image_source (image_part_id,
				image_filename, image_spi_load_address,
				image_spi_load_length, &use_tftp);

		SAVE_IMAGE_DEVICE_ID(image_dev_list[image_selected_device].device_id);
		SAVE_IMAGE_LOADADDR(image_dev_list[image_selected_device].load_address);

		if ( image_dev_list[image_selected_device].dev_type != DEV_TFTP && 
				image_dev_list[image_selected_device].dev_type != DEV_SPI) {
			SAVE_IMAGE_PATH(image_filename);
		}

		switch (image_dev_list[image_selected_device].dev_type) {
			case DEV_EMMC:
			case DEV_U_SD:
			case DEV_SATA:
			case DEV_USB:
				SAVE_IMAGE_PARTITION(image_part_id);
				break;
			case DEV_SPI:
				SAVE_IMAGE_SPI_ADDR(image_spi_load_address);
				SAVE_IMAGE_SPI_LEN(image_spi_load_length);
				break;
			case DEV_TFTP:
				break;
			default:
				break;
		}

		SAVE_IMAGE_BOOT_LOAD(image_dev_list[image_selected_device].env_str);

	}

/*  __________________________________________________________________________
 * |_______________________________ NET OF TFTP ______________________________|
 */
	if ( set_image ) { 
		if ( use_tftp ) {
			select_tftp_parameters (serverip_tftp , ipaddr_tftp);
			setenv ("serverip", serverip_tftp);
			setenv ("ipaddr", ipaddr_tftp);
		}	
	}
		
/*  __________________________________________________________________________
 * |______________________________ FILE SYSTEM _______________________________|
 */
	if ( set_fs ) { 

		filesystem_selected_device = select_filesystem_souce (filesystem_part_id,
				filesystem_path_nfs, filesystem_server_nfs, filesystem_ipaddr_nfs,
				filesystem_netmask_nfs, &filesystem_use_dhcp);

		switch (filesystem_dev_list[filesystem_selected_device].dev_type) {
			case DEV_EMMC:
			case DEV_U_SD:
				SAVE_FILESYSTEM_DEVICE_ID(filesystem_dev_list[filesystem_selected_device].device_id);
				SAVE_FILESYSTEM_PARTITION(filesystem_part_id);
				break;
			case DEV_USB:
				SAVE_FILESYSTEM_PARTITION(filesystem_part_id);
				break;
			case DEV_NFS:
				SAVE_NFS_USE("1");
				SAVE_NFS_IP_CLIENT(filesystem_ipaddr_nfs);
				SAVE_NFS_IP_SERVER(filesystem_server_nfs);
				SAVE_NFS_NETMASK(filesystem_netmask_nfs);
				SAVE_NFS_PATH(filesystem_path_nfs);
				if ( filesystem_use_dhcp == 1 ) {
					SAVE_NFS_USE_DHCP("1");
				} else {
					SAVE_NFS_USE_DHCP("0");
				}
				printf ("--- %s   %s   %s    %s\n", filesystem_path_nfs, filesystem_server_nfs, filesystem_ipaddr_nfs, filesystem_netmask_nfs);
				break;
			case DEV_TFTP:
			case DEV_SPI:
				break;
			default:
				break;
		}

		if ( filesystem_dev_list[filesystem_selected_device].dev_type != DEV_NFS )
			SAVE_NFS_USE("0");

		sprintf (filesystem_boot_string, "%s",
				filesystem_dev_list[filesystem_selected_device].env_str);

		SAVE_FILESYSTEM_ROOT(filesystem_boot_string);


	}

	printf ("\n\n");
	saveenv ();
	printf ("\n\n");

	return 0;
}

U_BOOT_CMD(
	bconfig, 3, 1, do_board_config,
	"Interactive setup for AXIOM configuration.",
	"      - set whole environment\n"
	"bconfig [default] - resetting to default environment\n"
	"bconfig [image]    - set image.ub source device.\n"
	"bconfig [fs]       - set FileSystem source device.\n"
);
