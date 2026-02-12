#ifndef SYSTEMSETTINGSWIDGET_H
#define SYSTEMSETTINGSWIDGET_H

#include <QWidget>
#include "LogManager.h"
#include "utils/ConfigManager.h"

class QLineEdit;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QLabel;
class QComboBox;

class SystemSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SystemSettingsWidget(QWidget *parent = nullptr);

    // 加载配置
    void loadSettings();
    // 保存配置
    void saveSettings();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void updatePermissionView();

private:
    QWidget *m_mainContentWrapper; // 包装原有的所有 UI
    QWidget *m_permissionMask;     // 权限提示层

    LogManager *logger = &LogManager::instance();   // 日志管理器
    ConfigManager *cfg = ConfigManager::instance(); // 系统参数

    void initUI();

    QLabel *createSectionLabel(const QString &text);

    // --- UI 控件成员变量 ---
    // AGV 参数
    QLineEdit *m_agvIdEdit;
    QLineEdit *m_agvIpEdit;
    QSpinBox *m_chargingThresholdBox;
    QComboBox *m_vehicleTypeCombo;
    QComboBox *m_mapResolutionCombo;
    QSpinBox *m_arcVwBox;
    QSpinBox *m_spinVwBox;

    // 文件路径
    QLineEdit *m_resourceFolderEdit;
    QLineEdit *m_mapPngFolderEdit;
    QLineEdit *m_mapJsonFolderEdit;
    QLineEdit *m_configFolderEdit;
    QLineEdit *m_logFolderEdit;

    // 网络设置
    QLineEdit *m_commIpEdit;
    QSpinBox *m_commPortBox;
    QLineEdit *m_truckLoadingIpEdit;
    QSpinBox *m_truckLoadingPortBox;
    QLineEdit *m_rosBridgeIpEdit;
    QSpinBox *m_rosBridgePortBox;
    QLineEdit *m_serverIpEdit;
    QSpinBox *m_serverPortBox;

    // 其他通讯
    QLineEdit *m_microControllerComEdit;
    QComboBox *m_microControllerComBaudrateCombo;

    // 系统选项
    QSpinBox *m_adminDurationBox;
    QCheckBox *m_defaultFixedRelocationCheck;
    QCheckBox *m_debugModeCheck;
    QCheckBox *m_fullScreenCheck;

    // 按钮
    QPushButton *m_saveBtn;
    QPushButton *m_exitBtn;
};

#endif // SYSTEMSETTINGSWIDGET_H