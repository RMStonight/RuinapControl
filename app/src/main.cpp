#include <QApplication>
#include "MainWindow.h"
#include <QIcon>
#include "utils/ConfigManager.h"
#include "Version.h"
#include "utils/GlobalEventFilter.h"

// 程序的任务栏/窗口左上角图标
#define ICON_LOGO "icon.png"

int main(int argc, char *argv[])
{
    // 初始化应用程序对象
    QApplication app(argc, argv);

    // 设置全局属性（可选）
    app.setOrganizationName("Ruinap");       // 设置组织名 (作为文件夹名)
    app.setOrganizationDomain("ruinap.com"); // 设置域名 (可选，有时用于 macOS/iOS)
    app.setApplicationName("RuinapControl");
    app.setApplicationVersion(APP_VERSION);

    // 在 UI 启动前，优先加载配置到内存
    ConfigManager::instance()->load();

    // 设置程序的任务栏/窗口左上角图标,改为从内存单例读取
    // 先加载图片
    ConfigManager *cfg = ConfigManager::instance();
    QString resourceFolder = cfg->resourceFolder();
    // 如果相对路径不是以 / 结尾则需要添加
    if (!resourceFolder.endsWith("/"))
    {
        resourceFolder += "/";
    }
    QPixmap iconPixmap(resourceFolder + ICON_LOGO);

    // 检查图片是否加载成功 (也是为了防止写错文件名)
    if (!iconPixmap.isNull())
    {
        // 强制缩小图片
        // 128 x 128 对窗口图标来说已经非常清晰了
        // 使用 SmoothTransformation 保证缩小后边缘平滑
        QIcon appIcon(iconPixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        // 设置缩小后的图标
        app.setWindowIcon(appIcon);
    }
    else
    {
        // 注意：这里用 stderr 打印，因为 qDebug 可能还没初始化好
        fprintf(stderr, "Warning: Failed to load window icon!\n");
    }

    // 实例化并安装全局事件过滤器
    GlobalEventFilter *filter = new GlobalEventFilter();
    app.installEventFilter(filter);

    // 创建并显示主窗口
    MainWindow w;
    w.show();

    // 进入事件循环（死循环，直到点击关闭）
    return app.exec();
}