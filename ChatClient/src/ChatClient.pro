QT += core gui network websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# 源文件
# SOURCES +=    model/AsyncAvatarLoader.cpp \
#     model/HttpApiClient.cpp \
#     model/MessageCacheManager.cpp \
#     model/UserSession.cpp \
#     model/WebSocketClient.cpp \
#     view/LoginPage.cpp \
#     view/MainPage.cpp \
#     view/RegisterPage.cpp \
#     view/chatmainpage.cpp
SOURCES += model/AsyncAvatarLoader.cpp model/HttpApiClient.cpp model/MessageCacheManager.cpp  model/UserSession.cpp  model/WebSocketClient.cpp view/LoginPage.cpp view/MainPage.cpp view/RegisterPage.cpp view/chatmainpage.cpp view/CustomDialog.cpp \


# 头文件
#HEADERS +=  model/AsyncAvatarLoader.h \
#    model/HttpApiClient.h \
#    model/MessageCacheManager.h \
#    model/UserSession.h \
#    model/WebSocketClient.h \
#    view/LoginPage.h \
#    view/MainPage.h \
#    view/RegisterPage.h \
#    view/chatmainpage.h
HEADERS += model/AsyncAvatarLoader.h  model/HttpApiClient.h  model/MessageCacheManager.h model/UserSession.h  model/WebSocketClient.h view/LoginPage.h view/MainPage.h view/RegisterPage.h view/chatmainpage.h view/CustomDialog.h \


# UI 表单
FORMS += \
    ui/LoginPage.ui \
    ui/RegisterPage.ui \
    ui/chatmainpage.ui

# 资源文件
RESOURCES += \
    MainPage.qrc

# 部署规则
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
