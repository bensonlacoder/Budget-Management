QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../src/balance.cpp \
    ../src/budget.cpp \
    ../src/main.cpp \
    ../src/prime.cpp \
    ../tests/test_balance.cpp \
    ../tests/test_budget.cpp \
    ../tests/test_main.cpp \
    ../tests/test_prime.cpp \
    main.cpp \
    mainui.cpp

HEADERS += \
    ../src/balance.h \
    ../src/budget.h \
    ../src/prime.h \
    ../tests/doctest.h \
    mainui.h

FORMS += \
    mainui.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
