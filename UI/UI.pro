QT += widgets network sql

CONFIG += c++17

INCLUDEPATH += ../src

SOURCES += \
    ../src/balance.cpp \
    ../src/budget.cpp \
    ../src/quarter.cpp \
    authdialog.cpp \
    chartspage.cpp \
    emailsender.cpp \
    friendsdialog.cpp \
    localaccounts.cpp \
    main.cpp \
    mainui.cpp \
    menusearchdialog.cpp \
    planpage.cpp \
    profilepage.cpp \
    purchasespage.cpp \
    lunchcatalog.cpp \
    scumenu.cpp

HEADERS += \
    ../src/balance.h \
    ../src/budget.h \
    ../src/quarter.h \
    authdialog.h \
    chartspage.h \
    emailsender.h \
    friendsdialog.h \
    localaccounts.h \
    mainui.h \
    menusearchdialog.h \
    planpage.h \
    profilepage.h \
    purchasespage.h \
    scumenu.h

FORMS += \
    mainui.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
