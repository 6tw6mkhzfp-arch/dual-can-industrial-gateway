#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>

struct ChannelView {
    QLabel *state = nullptr;
    QLabel *count = nullptr;
    QLabel *last = nullptr;
    quint64 frames = 0;
    qint64 lastSeenMs = 0;
};

class StatusWindow : public QMainWindow {
public:
    StatusWindow() {
        setWindowTitle(QStringLiteral("双 CAN 工业边缘网关"));
        setStyleSheet(QStringLiteral(
            "QWidget{background:#101827;color:#e8f0ff;font-family:sans-serif;}"
            "QLabel#title{font-size:34px;font-weight:700;color:#f3f7ff;}"
            "QLabel#clock{font-size:18px;color:#8da2c6;}"
            "QGroupBox{font-size:24px;font-weight:700;border:2px solid #2d466a;"
            "border-radius:18px;margin-top:20px;padding:22px;background:#162238;}"
            "QGroupBox::title{subcontrol-origin:margin;left:22px;padding:0 10px;}"
            "QLabel.state{font-size:32px;font-weight:800;}"
            "QLabel.info{font-size:20px;color:#c5d2e8;}"
            "QLabel.frame{font-family:monospace;font-size:18px;color:#8fc7ff;}"
            "QPushButton{background:#245a91;border:0;border-radius:12px;padding:14px;"
            "font-size:20px;font-weight:600;} QPushButton:pressed{background:#19436e;}"));

        QWidget *root = new QWidget;
        QVBoxLayout *layout = new QVBoxLayout(root);
        layout->setContentsMargins(28, 34, 28, 28);
        layout->setSpacing(22);

        QLabel *title = new QLabel(QStringLiteral("双 CAN 工业边缘网关"));
        title->setObjectName(QStringLiteral("title"));
        clock_ = new QLabel;
        clock_->setObjectName(QStringLiteral("clock"));
        layout->addWidget(title);
        layout->addWidget(clock_);
        layout->addWidget(makeCard(QStringLiteral("CAN 1"), channels_[0]));
        layout->addWidget(makeCard(QStringLiteral("CAN 2"), channels_[1]));

        QGroupBox *telemetryBox = new QGroupBox(QStringLiteral("设备实时数据"));
        QVBoxLayout *telemetryLayout = new QVBoxLayout(telemetryBox);
        telemetry_ = new QLabel(QStringLiteral("等待设备数据…"));
        telemetry_->setProperty("class", QStringLiteral("info"));
        telemetry_->setWordWrap(true);
        telemetry_->setAlignment(Qt::AlignCenter);
        alarm_ = new QLabel(QStringLiteral("无报警"));
        alarm_->setProperty("class", QStringLiteral("state"));
        alarm_->setAlignment(Qt::AlignCenter);
        telemetryLayout->addWidget(telemetry_);
        telemetryLayout->addWidget(alarm_);
        layout->addWidget(telemetryBox);

        QLabel *hint = new QLabel(QStringLiteral(
            "在线判定：3 秒内收到任意有效 CAN 帧\n"
            "总线速率：500 kbit/s  ·  USB-CAN：0471:1200"));
        hint->setProperty("class", QStringLiteral("info"));
        hint->setAlignment(Qt::AlignCenter);
        hint->setWordWrap(true);
        layout->addWidget(hint);

        QPushButton *reset = new QPushButton(QStringLiteral("清零统计"));
        connect(reset, &QPushButton::clicked, this, [this] {
            for (ChannelView &channel : channels_) channel.frames = 0;
            updateCards();
        });
        layout->addWidget(reset);
        setCentralWidget(root);

        timer_ = new QTimer(this);
        connect(timer_, &QTimer::timeout, this, [this] { pollLog(); });
        timer_->start(250);
        updateCards();
    }

private:
    QGroupBox *makeCard(const QString &name, ChannelView &channel) {
        QGroupBox *box = new QGroupBox(name);
        QVBoxLayout *layout = new QVBoxLayout(box);
        layout->setSpacing(12);
        channel.state = new QLabel(QStringLiteral("OFFLINE"));
        channel.state->setProperty("class", QStringLiteral("state"));
        channel.count = new QLabel(QStringLiteral("接收帧数：0"));
        channel.count->setProperty("class", QStringLiteral("info"));
        channel.last = new QLabel(QStringLiteral("最后一帧：—"));
        channel.last->setProperty("class", QStringLiteral("frame"));
        channel.last->setWordWrap(true);
        layout->addWidget(channel.state);
        layout->addWidget(channel.count);
        layout->addWidget(channel.last);
        return box;
    }

    void pollLog() {
        const QString path = QStringLiteral("/var/log/usbcan_gateway.log");
        QFileInfo info(path);
        if (!info.exists()) {
            updateCards();
            return;
        }
        if (offset_ > info.size()) offset_ = 0;
        if (!initialized_) {
            offset_ = info.size();
            initialized_ = true;
        }
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text) && file.seek(offset_)) {
            while (!file.atEnd()) parseLine(QString::fromUtf8(file.readLine()).trimmed());
            offset_ = file.pos();
        }
        updateCards();
    }

    void parseLine(const QString &line) {
        static const QRegularExpression canRe(QStringLiteral("(?:^|\\s)can=([12])(?:\\s|$)"));
        static const QRegularExpression idRe(QStringLiteral("(?:^|\\s)id=([0-9A-Fa-f]+)"));
        static const QRegularExpression dataRe(QStringLiteral("(?:^|\\s)data=([^\\r\\n]+?)(?:\\s+can=[12])?$"));
        const QRegularExpressionMatch canMatch = canRe.match(line);
        const QRegularExpressionMatch idMatch = idRe.match(line);
        if (!canMatch.hasMatch() || !idMatch.hasMatch()) return;
        const int index = canMatch.captured(1).toInt() - 1;
        if (index < 0 || index > 1) return;
        ChannelView &channel = channels_[index];
        channel.frames++;
        channel.lastSeenMs = QDateTime::currentMSecsSinceEpoch();
        const QString data = dataRe.match(line).captured(1).trimmed();
        channel.last->setText(QStringLiteral("最后一帧：ID 0x%1  %2")
                              .arg(idMatch.captured(1).toUpper(), data));
        bool idOk = false;
        const uint id = idMatch.captured(1).toUInt(&idOk, 16);
        QStringList fields = data.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (!idOk || fields.size() < 8) return;
        uint8_t bytes[8] = {0};
        for (int i = 0; i < 8; ++i) {
            bool ok = false;
            bytes[i] = static_cast<uint8_t>(fields[i].toUInt(&ok, 16));
            if (!ok) return;
        }
        telemetryChannel_ = index;
        if (id == 0x101U) {
            temperature_ = static_cast<qint16>(bytes[0] | (bytes[1] << 8)) / 10.0;
            voltage_ = static_cast<quint16>(bytes[2] | (bytes[3] << 8)) / 1000.0;
            current_ = static_cast<quint16>(bytes[4] | (bytes[5] << 8)) / 1000.0;
            humidity_ = static_cast<quint16>(bytes[6] | (bytes[7] << 8)) / 10.0;
        } else if (id == 0x102U) {
            speed_ = static_cast<quint16>(bytes[0] | (bytes[1] << 8));
            pressure_ = static_cast<quint16>(bytes[2] | (bytes[3] << 8)) / 10.0;
            load_ = bytes[4];
        } else if (id == 0x200U) {
            alarmActive_ = bytes[0] != 0 || bytes[1] != 0 ||
                           bytes[2] != 0 || bytes[3] != 0;
        }
    }

    void updateCards() {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        clock_->setText(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd  HH:mm:ss")));
        for (ChannelView &channel : channels_) {
            const bool online = channel.lastSeenMs > 0 && now - channel.lastSeenMs <= 3000;
            channel.state->setText(online ? QStringLiteral("ONLINE") : QStringLiteral("OFFLINE"));
            channel.state->setStyleSheet(online ? QStringLiteral("color:#35e58b;")
                                                : QStringLiteral("color:#ff6577;"));
            channel.count->setText(QStringLiteral("接收帧数：%1").arg(channel.frames));
        }
        if (telemetryChannel_ >= 0) {
            telemetry_->setText(QStringLiteral(
                "CAN %1  ·  温度 %2 °C  ·  湿度 %3 %\n"
                "电压 %4 V  ·  电流 %5 A\n"
                "转速 %6 rpm  ·  压力 %7 kPa  ·  环境亮度 %8 %")
                .arg(telemetryChannel_ + 1)
                .arg(temperature_, 0, 'f', 1).arg(humidity_, 0, 'f', 1)
                .arg(voltage_, 0, 'f', 2).arg(current_, 0, 'f', 2)
                .arg(speed_).arg(pressure_, 0, 'f', 1).arg(load_));
            alarm_->setText(alarmActive_ ? QStringLiteral("超温报警") : QStringLiteral("运行正常 · 无报警"));
            alarm_->setStyleSheet(alarmActive_ ? QStringLiteral("color:#ff6577;")
                                                : QStringLiteral("color:#35e58b;"));
        }
    }

    ChannelView channels_[2];
    QLabel *clock_ = nullptr;
    QLabel *telemetry_ = nullptr;
    QLabel *alarm_ = nullptr;
    QTimer *timer_ = nullptr;
    qint64 offset_ = 0;
    bool initialized_ = false;
    int telemetryChannel_ = -1;
    double temperature_ = 0.0;
    double voltage_ = 0.0;
    double current_ = 0.0;
    double humidity_ = 0.0;
    int speed_ = 0;
    double pressure_ = 0.0;
    int load_ = 0;
    bool alarmActive_ = false;
};

int main(int argc, char **argv) {
    if (argc == 2 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--self-test"))
        return 0;
    QApplication app(argc, argv);
    StatusWindow window;
    window.showFullScreen();
    return app.exec();
}
