#!/sbin/sh
# $Id: smf-vboxwebsrv.sh 36748 2011-04-20 11:24:54Z vboxsync $

# Copyright (C) 2008-2011 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

#
# smf-vboxwebsrv method
#
# Argument is the method name (start, stop, ...)

. /lib/svc/share/smf_include.sh

VW_OPT="$1"
VW_EXIT=0

case $VW_OPT in
    start)
        if [ ! -x /opt/VirtualBox/vboxwebsrv ]; then
            echo "ERROR: /opt/VirtualBox/vboxwebsrv does not exist."
            return $SMF_EXIT_ERR_CONFIG
        fi

        if [ ! -f /opt/VirtualBox/vboxwebsrv ]; then
            echo "ERROR: /opt/VirtualBox/vboxwebsrv does not exist."
            return $SMF_EXIT_ERR_CONFIG
        fi

        # Get svc configuration
        VW_USER=`/usr/bin/svcprop -p config/user $SMF_FMRI 2>/dev/null`
        [ $? != 0 ] && VW_USER=
        VW_HOST=`/usr/bin/svcprop -p config/host $SMF_FMRI 2>/dev/null`
        [ $? != 0 ] && VW_HOST=
        VW_PORT=`/usr/bin/svcprop -p config/port $SMF_FMRI 2>/dev/null`
        [ $? != 0 ] && VW_PORT=
        VW_TIMEOUT=`/usr/bin/svcprop -p config/timeout $SMF_FMRI 2>/dev/null`
        [ $? != 0 ] && VW_TIMEOUT=
        VW_CHECK_INTERVAL=`/usr/bin/svcprop -p config/checkinterval $SMF_FMRI 2>/dev/null`
        [ $? != 0 ] && VW_CHECK_INTERVAL=
        VW_KEEPALIVE=`/usr/bin/svcprop -p config/keepalive $SMF_FMRI 2>/dev/null`
        [ $? != 0 ] && VW_KEEPALIVE=
        VW_ROTATE=`/usr/bin/svcprop -p config/logrotate $SMF_FMRI 2>/dev/null`
        [ $? != 0 ] && VW_ROTATE=
        VW_LOGSIZE=`/usr/bin/svcprop -p config/logsize $SMF_FMRI 2>/dev/null`
        [ $? != 0 ] && VW_LOGSIZE=
        VW_LOGINTERVAL=`/usr/bin/svcprop -p config/loginterval $SMF_FMRI 2>/dev/null`
        [ $? != 0 ] && VW_LOGINTERVAL=

        # Provide sensible defaults
        [ -z "$VW_USER" ] && VW_USER=root
        [ -z "$VW_HOST" ] && VW_HOST=localhost
        [ -z "$VW_PORT" -o "$VW_PORT" -eq 0 ] && VW_PORT=18083
        [ -z "$VW_TIMEOUT" ] && VW_TIMEOUT=20
        [ -z "$VW_CHECK_INTERVAL" ] && VW_CHECK_INTERVAL=5
        [ -z "$VW_KEEPALIVE" ] && VW_KEEPALIVE=100
        [ -z "$VW_ROTATE" ] && VW_ROTATE=10
        [ -z "$VW_LOGSIZE" ] && VW_LOGSIZE=104857600
        [ -z "$VW_LOGINTERVAL" ] && VW_LOGINTERVAL=86400
        exec su - "$VW_USER" -c "/opt/VirtualBox/vboxwebsrv --background --host \"$VW_HOST\" --port \"$VW_PORT\" --timeout \"$VW_TIMEOUT\" --check-interval \"$VW_CHECK_INTERVAL\" --keepalive \"$VW_KEEPALIVE\" --logrotate \"$VW_ROTATE\" --logsize \"$VW_LOGSIZE\" --loginterval \"$VW_LOGINTERVAL\""

        VW_EXIT=$?
        if [ $VW_EXIT != 0 ]; then
            echo "vboxwebsrv failed with $VW_EXIT."
            VW_EXIT=1
        fi
    ;;
    stop)
        # Kill service contract
        smf_kill_contract $2 TERM 1
    ;;
    *)
        VW_EXIT=$SMF_EXIT_ERR_CONFIG
    ;;
esac

exit $VW_EXIT
