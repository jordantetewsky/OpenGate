// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QWidget>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <atomic>

class AuditionTrace : public QObject
{
    Q_OBJECT
public:
    explicit AuditionTrace(QObject *parent=nullptr);
    bool begin(bool observe=true);
    void checkpoint();
    void finish(const QString &reason);
    bool active() const { return m_active; }
protected:
    bool eventFilter(QObject *object,QEvent *event) override;
private slots:
    void contextChanged(int previous,int current);
    void playerBool(bool value);
    void playerEvent();
private:
    void discover(bool observe);
    void connectSignal(QObject *object,const char *signature,const char *slot);
    void record(const QString &kind,QJsonObject data={});
    void snapshot(const QString &label);
    void flush();
    QJsonObject state() const;
    bool guiThread();
    QList<QPointer<QObject>> m_managers;
    QList<QPointer<QWidget>> m_players;
    QPointer<QWidget> m_main,m_page,m_slot;
    QList<QMetaObject::Connection> m_connections;
    QVector<QByteArray> m_records;
    QElapsedTimer m_clock;
    QTimer m_deadline;
    QString m_file;
    int m_count=0,m_checkpoint=0;
    bool m_active=false,m_finishing=false;
    std::atomic<int> m_offThread{0};
};
