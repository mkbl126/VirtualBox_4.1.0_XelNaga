/** @file
 *
 * VBox frontends: Qt4 GUI ("VirtualBox"):
 * VBoxUpdateDlg class declaration
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

#ifndef __VBoxUpdateDlg_h__
#define __VBoxUpdateDlg_h__

/* Global includes */
#include <QUrl>
#include <QDate>

/* Local includes */
#include "QIWithRetranslateUI.h"
#include "VBoxUpdateDlg.gen.h"

/* Global forwards */
class QIHttp;

/**
 *  This structure is used to store retranslated reminder values.
 */
struct UpdateDay
{
    UpdateDay (const QString &aVal, const QString &aKey)
        : val (aVal), key (aKey) {}

    QString val;
    QString key;

    bool operator== (const UpdateDay &aOther)
    {
        return val == aOther.val || key == aOther.key;
    }
};

/**
 *  This class is used to encode/decode the registration data.
 */
class VBoxUpdateData
{
public:

    enum PeriodType
    {
        PeriodNever     = -2,
        PeriodUndefined = -1,
        Period1Day      =  0,
        Period2Days     =  1,
        Period3Days     =  2,
        Period4Days     =  3,
        Period5Days     =  4,
        Period6Days     =  5,
        Period1Week     =  6,
        Period2Weeks    =  7,
        Period3Weeks    =  8,
        Period1Month    =  9
    };

    enum BranchType
    {
        BranchStable     = 0,
        BranchAllRelease = 1,
        BranchWithBetas  = 2
    };

    static void populate();
    static QStringList list();

    VBoxUpdateData (const QString &aData);
    VBoxUpdateData (PeriodType aPeriodIndex, BranchType aBranchIndex);

    bool isNecessary();
    bool isNoNeedToCheck();

    QString data() const;
    PeriodType periodIndex() const;
    QString date() const;
    BranchType branchIndex() const;
    QString branchName() const;

private:

    /* Private functions */
    void decode();
    void encode();

    /* Private variables */
    static QList <UpdateDay> mDayList;

    QString mData;
    PeriodType mPeriodIndex;
    QDate mDate;
    BranchType mBranchIndex;
};

class VBoxUpdateDlg : public QIWithRetranslateUI <QDialog>,
                      public Ui::VBoxUpdateDlg
{
    Q_OBJECT;

public:

    static bool isNecessary();

    VBoxUpdateDlg (VBoxUpdateDlg **aSelf, bool aForceRun, QWidget *aParent = 0);
   ~VBoxUpdateDlg();

public slots:

    void search();

protected:

    void retranslateUi();

private slots:

    void accept();
    void searchResponse (bool aError);

private:

    void abortRequest (const QString &aReason);

    /* Private variables */
    VBoxUpdateDlg **mSelf;
    QUrl            mUrl;
    QIHttp         *mHttp;
    bool            mForceRun;
};

#endif // __VBoxUpdateDlg_h__

