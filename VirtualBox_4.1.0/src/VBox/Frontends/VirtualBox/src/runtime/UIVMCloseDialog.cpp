/* $Id: UIVMCloseDialog.cpp 32483 2010-09-14 13:45:07Z vboxsync $ */
/** @file
 *
 * VBox frontends: Qt4 GUI ("VirtualBox"):
 * UIVMCloseDialog class implementation
 */

/*
 * Copyright (C) 2006-2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include "precomp.h"
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */
#include "UIVMCloseDialog.h"
#include "VBoxProblemReporter.h"
#include "UIMachineWindowNormal.h"

#ifdef Q_WS_MAC
# include "VBoxGlobal.h"
#endif /* Q_WS_MAC */

/* Qt includes */
#include <QPushButton>
#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

UIVMCloseDialog::UIVMCloseDialog(QWidget *pParent)
    : QIWithRetranslateUI<QIDialog>(pParent)
{
#ifdef Q_WS_MAC
    /* No sheets in another mode than normal for now. Firstly it looks ugly and
     * secondly in some cases it is broken. */
    if (vboxGlobal().isSheetWindowsAllowed(pParent))
        setWindowFlags(Qt::Sheet);
#endif /* Q_WS_MAC */

    /* Apply UI decorations */
    Ui::UIVMCloseDialog::setupUi(this);

#ifdef Q_WS_MAC
    /* Make some more space around the content */
    hboxLayout->setContentsMargins(40, 0, 40, 0);
    vboxLayout2->insertSpacing(1, 20);
    /* and more space between the radio buttons */
    gridLayout->setSpacing(15);
#endif /* Q_WS_MAC */
    /* Set fixed size */
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    connect(mButtonBox, SIGNAL(helpRequested()),
            &vboxProblem(), SLOT(showHelpHelpDialog()));
}

void UIVMCloseDialog::retranslateUi()
{
    /* Translate uic generated strings */
    Ui::UIVMCloseDialog::retranslateUi(this);
}

