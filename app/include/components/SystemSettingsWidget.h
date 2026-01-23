#ifndef SYSTEMSETTINGSWIDGET_H
#define SYSTEMSETTINGSWIDGET_H

#include <QWidget>

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

private:
    void initUI();

    QLabel* createSectionLabel(const QString &text);

    // --- UI 控件成员变量 ---
    // AGV 参数
    QLineEdit *m_agvIdEdit;
    QLineEdit *m_agvIpEdit;
    QSpinBox *m_maxSpeedBox;
    QComboBox *m_vehicleTypeCombo;
    QComboBox *m_mapResolutionCombo;

    // 文件路径
    QLineEdit *m_resourceFolderEdit;
    QLineEdit *m_mapPngFolderEdit;
    QLineEdit *m_configFolderEdit;

    // 网络设置
    QLineEdit *m_commIpEdit;
    QSpinBox *m_commPortBox;
    QLineEdit *m_rosBridgeIpEdit;
    QSpinBox *m_rosBridgePortBox;
    QLineEdit *m_serverIpEdit;
    QSpinBox *m_serverPortBox;
    
    // 系统选项
    QCheckBox *m_autoConnectCheck;
    QCheckBox *m_debugModeCheck;
    QCheckBox *m_fullScreenCheck;

    // 按钮
    QPushButton *m_saveBtn;
    QPushButton *m_exitBtn;
};

#endif // SYSTEMSETTINGSWIDGET_H