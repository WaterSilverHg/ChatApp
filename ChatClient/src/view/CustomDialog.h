#pragma once

#include "../global.h"

class CustomDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomDialog(QWidget* parent = nullptr);
    explicit CustomDialog(const QString& title, QWidget* parent = nullptr);
    ~CustomDialog();

    static void information(QWidget* parent, const QString& title, const QString& text);
    static void warning(QWidget* parent, const QString& title, const QString& text);
    static void critical(QWidget* parent, const QString& title, const QString& text);
    static bool question(QWidget* parent, const QString& title, const QString& text);

private:
    void setupStyle();
};

class StyledMessageBox : public QMessageBox
{
    Q_OBJECT

public:
    explicit StyledMessageBox(QWidget* parent = nullptr);
    explicit StyledMessageBox(Icon icon, const QString& title, const QString& text,
                              StandardButtons buttons = Ok, QWidget* parent = nullptr);
};
