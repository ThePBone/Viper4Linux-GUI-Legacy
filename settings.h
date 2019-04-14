#ifndef SETTINGS_H
#define SETTINGS_H

#include <pulse/pulseaudio.h>
#include <QDialog>
#include "ui_settings.h"
class settings : public QDialog
{
    Q_OBJECT

public:
    settings(QWidget *parent = nullptr);
    Ui::settings *ui;
    ~settings();
private slots:
        void submit();
        void reject();
        void github();
};

#endif // SETTINGS_H
