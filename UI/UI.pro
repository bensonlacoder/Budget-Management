QT += widgets network sql

CONFIG += c++17

INCLUDEPATH += ../src

SOURCES += \
    ../src/balance.cpp \
    ../src/budget.cpp \
    ../src/quarter.cpp \
    ../src/habitscsv.cpp \
    appstyle.cpp \
    authdialog.cpp \
    chartspage.cpp \
    emailsender.cpp \
    friendspage.cpp \
    localaccounts.cpp \
    main.cpp \
    mainui.cpp \
    planpage.cpp \
    profilepage.cpp \
    purchasespage.cpp \
    lunchcatalog.cpp \
    scumenu.cpp

HEADERS += \
    ../src/balance.h \
    ../src/budget.h \
    ../src/quarter.h \
    ../src/habitscsv.h \
    appstyle.h \
    authdialog.h \
    chartspage.h \
    emailsender.h \
    friendspage.h \
    localaccounts.h \
    mainui.h \
    planpage.h \
    profilepage.h \
    purchasespage.h \
    scumenu.h

FORMS += \
    mainui.ui

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
