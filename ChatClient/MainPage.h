#pragma once

#include <QtWidgets/QWidget>
#include "ui_MainPage.h"

class MainPage : public QWidget
{
    Q_OBJECT

public:
    MainPage(QWidget *parent = nullptr);
    ~MainPage();

private:
    Ui::MainPageClass ui;
};
