QT += core gui network websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    HttpApiClient.cpp \
    WebSocketClient.cpp \
    UserSession.cpp \
    LoginPage.cpp \
    RegisterPage.cpp \
    chatmainpage.cpp \
    main.cpp

HEADERS += \
    HttpApiClient.h \
    WebSocketClient.h \
    UserSession.h \
    LoginPage.h \
    RegisterPage.h \
    chatmainpage.h \
    global.h

FORMS += \
    LoginPage.ui \
    RegisterPage.ui \
    chatmainpage.ui

RESOURCES += \
    MainPage.qrc

# 部署规则
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target