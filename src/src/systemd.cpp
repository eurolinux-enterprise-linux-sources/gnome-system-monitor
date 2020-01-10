#include <config.h>
#include <stdlib.h>

#ifdef HAVE_SYSTEMD
#include <systemd/sd-login.h>
#endif

#include "application.h"
#include "systemd.h"


bool
procman::systemd_logind_running()
{
#ifdef HAVE_SYSTEMD
  static bool init;
  static bool is_running;

  if (!init) {
    /* check if logind is running */
    if (access("/run/systemd/seats/", F_OK) >= 0) {
      is_running = true;
    }
    init = true;
  }

  return is_running;

#else
  return false;
#endif
}

void
procman::get_process_systemd_info(ProcInfo *info)
{
#ifdef HAVE_SYSTEMD
    uid_t uid;

    if (!systemd_logind_running())
        return;

    free(info->unit);
    info->unit = NULL;
    sd_pid_get_unit(info->pid, &info->unit);

    free(info->session);
    info->session = NULL;
    sd_pid_get_session(info->pid, &info->session);

    free(info->seat);
    info->seat = NULL;

    if (info->session != NULL)
        sd_session_get_seat(info->session, &info->seat);

    if (sd_pid_get_owner_uid(info->pid, &uid) >= 0)
        info->owner = info->lookup_user(uid);
    else
        info->owner = "";
#endif
}
