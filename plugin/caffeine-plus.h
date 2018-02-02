#ifndef CAFFEINE_PLUS_H
#define CAFFEINE_PLUS_H

#include <QUrl>
#include <QObject>
#include <QVariantMap>
#include <KWindowInfo>
#include <KWindowSystem>

class QDBusServiceWatcher;

using InhibitionInfo = QPair<QString, QString>;
using InhibitionInfoCaffeinePlus = QPair<QString, uint>;

class CaffeinePlus : public QObject
{
    Q_OBJECT

public:
	CaffeinePlus(QObject *parent = nullptr);
	virtual ~CaffeinePlus();

	///////////////////////////////////////////////////

	Q_INVOKABLE QVariantMap launcherData(const QUrl &url);
	Q_INVOKABLE void openUrl(const QUrl &url);
	Q_INVOKABLE void openExec(const QString &exec);

	Q_INVOKABLE void addLauncher(bool isPopup = false);

Q_SIGNALS:
	void launcherAdded(const QString &url, bool isPopup);
	///////////////////////////////////////////////////

public Q_SLOTS:
	void init(bool enableFullscreen, const QStringList &userApps);
	void checkInhibition();

    bool isInhibited() const {
    	return m_inhibited;
    }

    void addInhibition(const QString &appName, const QString &reason);
    void releaseInhibition(const QString &appName);

private Q_SLOTS:
	void inhibitionsChanged(const QList<InhibitionInfo> &added, const QStringList &removed);
	void windowAdded (WId id);
	void windowChanged (WId id, NET::Properties properties, NET::Properties2 properties2);
	void windowRemoved (WId id);

private:
//	void checkInhibition();
	void update();
	void inhibitFullscreen(WId id, bool isFullScreen);
	void inhibitUserApps(WId id);
	void listenWindows();

	QDBusServiceWatcher *m_solidPowerServiceWatcher;
	bool m_serviceRegistered = false;
	bool m_inhibited = false;
	bool m_isInited = false;
	QList<InhibitionInfoCaffeinePlus> m_apps; // for caffeine, first item is app_name, second is cookie

	QStringList m_userApps; // for caffeine, first item is app_name, second is cookie
	bool m_enableFullscreen; // for caffeine, first item is app_name, second is cookie
};

#endif // CAFFEINE_PLUS_H
