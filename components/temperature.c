/* See LICENSE file for copyright and license details. */
#include <stddef.h>
#include <stdlib.h> /* for strtod */
#include <unistd.h> /* for access and F_OK */
#include <stdio.h>   /* Add this line for FILE, fopen, fscanf, fclose, popen, fgets, pclose */
#include <string.h>  /* Add this line for strchr */
#include "../slstatus.h"
#include "../util.h"

#if defined(__linux__)
#include <stdint.h>

const char *temp(const char *input) {
  
  uintmax_t temp;
  FILE *fp;
  char buf[BUFSIZ];

  // Try to open the input as a file
  fp = fopen(input, "r");
  if (fp) {
    // If fopen succeeds, it's a file path
    if (fscanf(fp, "%ju", &temp) != 1) {
      fclose(fp);
      return NULL;
    }
    fclose(fp);
    // Convert the temperature to a human-readable format
    printf("Returning temperature: %s\n", bprintf("%ju", temp / 1000));
    return bprintf("%ju", temp / 1000);
  } else {
    // If fopen fails, treat the input as a shell command
    if (!(fp = popen(input, "r"))) {
      warn("popen '%s':", input);
      return NULL;
    }

    // Read the output of the shell command
    if (!fgets(buf, sizeof(buf), fp)) {
      pclose(fp);
      return NULL;
    }
    if (pclose(fp) < 0) {
      warn("pclose '%s':", input);
      return NULL;
    }

    // Remove the newline character if present
    char *newline = strchr(buf, '\n');
    if (newline)
      *newline = '\0';

    // Return the result of the shell command
    // printf("Returning temperature: %s\n", buf);
    // Convert the string to float
    char *end;
    float temp_float = strtof(buf, &end);

    // If conversion was successful, return the float, else return NULL
    if (end != buf) {
      return bprintf("%.1f", temp_float);
    } else {
      return NULL;
    }


  }
}

#elif defined(__OpenBSD__)
#include <stdio.h>
#include <sys/sensors.h>
#include <sys/sysctl.h>
#include <sys/time.h> /* before <sys/sensors.h> for struct timeval */

const char *temp(const char *unused) {
  int mib[5];
  size_t size;
  struct sensor temp;

  mib[0] = CTL_HW;
  mib[1] = HW_SENSORS;
  mib[2] = 0; /* cpu0 */
  mib[3] = SENSOR_TEMP;
  mib[4] = 0; /* temp0 */

  size = sizeof(temp);

  if (sysctl(mib, 5, &temp, &size, NULL, 0) < 0) {
    warn("sysctl 'SENSOR_TEMP':");
    return NULL;
  }

  /* kelvin to celsius */
  return bprintf("%d", (int)((float)(temp.value - 273150000) / 1E6));
}
#elif defined(__FreeBSD__)
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysctl.h>

#define ACPI_TEMP "hw.acpi.thermal.%s.temperature"

const char *temp(const char *zone) {
  char buf[256];
  int temp;
  size_t len;

  len = sizeof(temp);
  snprintf(buf, sizeof(buf), ACPI_TEMP, zone);
  if (sysctlbyname(buf, &temp, &len, NULL, 0) < 0 || !len)
    return NULL;

  /* kelvin to decimal celcius */
  return bprintf("%d.%d", (temp - 2731) / 10, abs((temp - 2731) % 10));
}
#endif
