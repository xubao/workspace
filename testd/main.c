#include <stdio.h>
#include <systemd/sd-daemon.h>
#include <systemd/sd-journal.h>
#include <systemd/sd-id128.h>

int main(int argc, char *argv[])
{
	printf("Start testd...\n");
	sleep(5);
	
	/* sd_notify */
	sd_notify(0, "READY=1\n"
		"STATUS=I am dead.\n");

	/* sd_journal */
	sd_journal_print(LOG_INFO, "Hello1, this is PID %lu", (unsigned long) getpid());
	sd_journal_send("MESSAGE=Hello2, this is PID %lu", (unsigned long) getpid(),
			"MESSAGE_ID=xubao",
			"_SYSTEMD_UNIT=testd.service",
			"PRIORITY=%i", LOG_ERR,
			NULL);

	/* sd_id128 */
	sd_id128_t id;
	id = SD_ID128_MAKE(ee,89,be,71,bd,6e,43,d6,91,e6,c5,5d,eb,03,02,07);
	puts(SD_ID128_CONST_STR(id));
	printf("The ID is " SD_ID128_FORMAT_STR ".\n", SD_ID128_FORMAT_VAL(id));	
	
	sd_id128_t id2;
	sd_id128_randomize(&id2);
	puts(SD_ID128_CONST_STR(id2));		

	printf("Eend testd...\n");	
	return 0;
}
