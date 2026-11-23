QT += widgets network sql

CONFIG += c++17

INCLUDEPATH += ../src

SOURCES += \
    ../src/balance.cpp \
    ../src/budget.cpp \
    authdialog.cpp \
    friendsdialog.cpp \
    localaccounts.cpp \
    main.cpp \
    mainui.cpp \
    purchasespage.cpp \
    lunchcatalog.cpp \
    scumenu.cpp

HEADERS += \
    ../src/balance.h \
    ../src/budget.h \
    authdialog.h \
    friendsdialog.h \
    localaccounts.h \
    mainui.h \
    purchasespage.h \
    scumenu.h

FORMS += \
    mainui.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
