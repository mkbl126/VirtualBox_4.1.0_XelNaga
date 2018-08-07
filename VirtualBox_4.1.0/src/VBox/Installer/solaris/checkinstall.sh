#!/bin/sh
## @file
#
# VirtualBox checkinstall script for Solaris.
#

#
# Copyright (C) 2009-2010 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

infoprint()
{
    echo 1>&2 "$1"
}

errorprint()
{
    echo 1>&2 "## $1"
}

abort_error()
{
    errorprint "Please close all VirtualBox processes and re-run this installer."
    errorprint "Note: It can take up to 10 seconds for all VirtualBox & related processes to close."
    exit 1
}

# nothing to check for remote install
if test "x${PKG_INSTALL_ROOT:=/}" != "x/"; then
    exit 0
fi

# Check if the Zone Access service is holding open vboxdrv, if so stop & remove it
servicefound=`svcs -H "svc:/application/virtualbox/zoneaccess" 2> /dev/null | grep '^online'`
if test ! -z "$servicefound"; then
    infoprint "VirtualBox's zone access service appears to still be running."
    infoprint "Halting & removing zone access service..."
    /usr/sbin/svcadm disable -s svc:/application/virtualbox/zoneaccess
    # Don't delete the service, handled by manifest class action
    # /usr/sbin/svccfg delete svc:/application/virtualbox/zoneaccess
fi

# Check if the Web service is running, if so stop & remove it
servicefound=`svcs -H "svc:/application/virtualbox/webservice" 2> /dev/null | grep '^online'`
if test ! -z "$servicefound"; then
    infoprint "VirtualBox web service appears to still be running."
    infoprint "Halting & removing webservice..."
    /usr/sbin/svcadm disable -s svc:/application/virtualbox/webservice
    # Don't delete the service, handled by manifest class action
    # /usr/sbin/svccfg delete svc:/application/virtualbox/webservice
fi

# Check if VBoxSVC is currently running
VBOXSVC_PID=`ps -eo pid,fname | grep VBoxSVC | grep -v grep | awk '{ print $1 }'`
if test ! -z "$VBOXSVC_PID" && test "$VBOXSVC_PID" -ge 0; then
    errorprint "VirtualBox's VBoxSVC (pid $VBOXSVC_PID) still appears to be running."
    abort_error
fi

# Check if VBoxNetDHCP is currently running
VBOXNETDHCP_PID=`ps -eo pid,fname | grep VBoxNetDHCP | grep -v grep | awk '{ print $1 }'`
if test ! -z "$VBOXNETDHCP_PID" && test "$VBOXNETDHCP_PID" -ge 0; then
    errorprint "VirtualBox's VBoxNetDHCP (pid $VBOXNETDHCP_PID) still appears to be running."
    abort_error
fi

# Check if vboxnet is still plumbed, if so try unplumb it
BIN_IFCONFIG=`which ifconfig 2> /dev/null`
if test -x "$BIN_IFCONFIG"; then
    vboxnetup=`$BIN_IFCONFIG vboxnet0 >/dev/null 2>&1`
    if test "$?" -eq 0; then
        infoprint "VirtualBox NetAdapter is still plumbed"
        infoprint "Trying to remove old NetAdapter..."
        $BIN_IFCONFIG vboxnet0 unplumb
        if test "$?" -ne 0; then
            errorprint "VirtualBox NetAdapter 'vboxnet0' couldn't be unplumbed (probably in use)."
            abort_error
        fi
    fi
    vboxnetup=`$BIN_IFCONFIG vboxnet0 inet6 >/dev/null 2>&1`
    if test "$?" -eq 0; then
        infoprint "VirtualBox NetAdapter (Ipv6) is still plumbed"
        infoprint "Trying to remove old NetAdapter..."
        $BIN_IFCONFIG vboxnet0 inet6 unplumb
        if test "$?" -ne 0; then
            errorprint "VirtualBox NetAdapter 'vboxnet0' IPv6 couldn't be unplumbed (probably in use)."
            abort_error
        fi
    fi
fi

exit 0

