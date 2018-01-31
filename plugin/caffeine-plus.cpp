#include "caffeine-plus.h"
#include "policyagentadaptor.h"
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

CaffeinePlus::CaffeinePlus(QObject *parent) : QObject(parent) {
}

CaffeinePlus::~CaffeinePlus() {
}

QString CaffeinePlus::checkName(){
    return "from plugin######################";
}

void CaffeinePlus::init() {
	 // Start the Policy Agent service
//	 new PolicyAgentAdaptor(CaffeinePlus::PolicyAgent::instance());

	 QDBusConnection::sessionBus().registerService("org.kde.Solid.PowerManagement.PolicyAgent");
//	 QDBusConnection::sessionBus().registerObject("/org/kde/Solid/PowerManagement/PolicyAgent", CaffeinePlus::PolicyAgent::instance());
}

uint CaffeinePlus::AddInhibition(uint in0, const QString &in1, const QString &in2)
{
    // handle method call org.kde.Solid.PowerManagement.PolicyAgent.AddInhibition
    return 0, 0, 0;
}

bool CaffeinePlus::HasInhibition(uint in0)
{
    // handle method call org.kde.Solid.PowerManagement.PolicyAgent.HasInhibition
    return 0;
}

QList<InhibitionInfo> CaffeinePlus::ListInhibitions()
{
	QList<InhibitionInfo> list;
    // handle method call org.kde.Solid.PowerManagement.PolicyAgent.ListInhibitions
    return list;
}

void CaffeinePlus::ReleaseInhibition(uint in0)
{
    // handle method call org.kde.Solid.PowerManagement.PolicyAgent.ReleaseInhibition

}
