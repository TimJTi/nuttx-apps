/****************************************************************************
 * apps/boot/nxboot/nxboot_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <nxboot.h>
#include <sys/boardctl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_NXBOOT_USE_EXT_ERROR_FN
#define errhdlr(err) nxboot_errhdlr(err)
#else
#  define errhdlr(err)
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

#ifdef CONFIG_NXBOOT_USE_EXT_ERROR_FN
extern void nxboot_errhdlr(int err);
#endif

static void progress(void);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

void progress(void)
{
  nxboot_print_info(".");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nxboot_main
 *
 * Description:
 *   NuttX bootlaoder entry point.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct boardioc_boot_info_s info;
  bool check_only;
  int ret;
  #ifdef CONFIG_NXBOOT_SWRESET_ONLY
  FAR struct boardioc_reset_cause_s cause;
#endif

#if defined(CONFIG_BOARDCTL) && !defined(CONFIG_NSH_ARCHINIT)
  /* Perform architecture-specific initialization (if configured) */

  boardctl(BOARDIOC_INIT, 0);

#ifdef CONFIG_BOARDCTL_FINALINIT
  /* Perform architecture-specific final-initialization (if configured) */

  boardctl(BOARDIOC_FINALINIT, 0);
#endif
#endif

  nxboot_log(LOG_NOTICE, "*** nxboot ***\n");

#ifdef CONFIG_NXBOOT_SWRESET_ONLY
  check_only = true;
  ret = boardctl(BOARDIOC_RESET_CAUSE, (uintptr_t)&cause);
  if (ret >= 0)
    {
      if (cause.cause == BOARDIOC_RESETCAUSE_CPU_SOFT ||
          cause.cause == BOARDIOC_RESETCAUSE_CPU_RWDT ||
          cause.cause == BOARDIOC_RESETCAUSE_PIN)
        {
          check_only = false;
        }
      else
        {
          nxboot_log(LOG_INFO, "Power reset detected, "
                                  "performing check only.\n");
        }
    }
#else
  check_only = false;
#endif

  ret = nxboot_perform_update(check_only, progress);
  if (ret < 0)
    {
      nxboot_log(LOG_INFO, "Power reset detected, "
                              "performing check only.\n");
      errhdlr(ret);
      return OK;
    }

  nxboot_log(LOG_INFO, "Found bootable image, boot from primary.\n");

  /* Call board specific image boot */

  info.path        = CONFIG_NXBOOT_PRIMARY_SLOT_PATH;
  info.header_size = CONFIG_NXBOOT_HEADER_SIZE;

  ret = boardctl(BOARDIOC_BOOT_IMAGE, (uintptr_t)&info);

  /* We only get here if the board failed to boot the image */

  errhdlr(ret);

  return ret;
}
