#ifndef CAFFEINE_PLUS_H
#define CAFFEINE_PLUS_H

#include <QObject>

class QDBusServiceWatcher;

using InhibitionInfo = QPair<QString, QString>;

class CaffeinePlus : public QObject
{
    Q_OBJECT

public:
	CaffeinePlus(QObject *parent = nullptr);
	virtual ~CaffeinePlus();

public Q_SLOTS:
	void checkInhibition();

    bool isInhibited() const {
    	return m_inhibited;
    }

private Q_SLOTS:
	void inhibitionsChanged(const QList<InhibitionInfo> &added, const QStringList &removed);

private:
//	void checkInhibition();
	void update();

	QDBusServiceWatcher *m_solidPowerServiceWatcher;
	bool m_serviceRegistered = false;
	bool m_inhibited = false;
};

#endif // CAFFEINE_PLUS_H
