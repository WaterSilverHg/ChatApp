QT += core gui network websockets widgets

CONFIG += c++17

SOURCES += \
    src/main.cpp \
    src/model/HttpApiClient.cpp \
    src/model/WebSocketClient.cpp \
    src/model/UserSession.cpp \
    src/model/AsyncAvatarLoader.cpp \
    src/model/MessageCacheManager.cpp \
    src/view/CustomDialog.cpp \
    src/view/LoginPage.cpp \
    src/view/RegisterPage.cpp \
    src/view/chatmainpage.cpp \
    src/view/SearchDialog.cpp \
    src/view/RequestDialog.cpp \
    src/view/InfoDialog.cpp \
    src/view/MainPage.cpp

HEADERS += \
    src/global.h \
    src/model/HttpApiClient.h \
    src/model/WebSocketClient.h \
    src/model/UserSession.h \
    src/model/AsyncAvatarLoader.h \
    src/model/MessageCacheManager.h \
    src/view/CustomDialog.h \
    src/view/LoginPage.h \
    src/view/RegisterPage.h \
    src/view/chatmainpage.h \
    src/view/SearchDialog.h \
    src/view/RequestDialog.h \
    src/view/InfoDialog.h \
    src/view/MainPage.h

FORMS += \
    src/ui/LoginPage.ui \
    src/ui/RegisterPage.ui \
    src/ui/chatmainpage.ui \
    src/ui/SearchDialog.ui \
    src/ui/RequestDialog.ui \
    src/ui/MainPage.ui

RESOURCES += \
    src/MainPage.qrc

INCLUDEPATH += src

# 部署规则
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
