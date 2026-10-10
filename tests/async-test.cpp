#include "async.hpp"
#include "http.hpp"
#include <QtConcurrent/QtConcurrentRun>
#include <QCoreApplication>
#include <QTimer>
#include <QFutureWatcher>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QTcpServer>
int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	int errors = 0;
	auto f = QtConcurrent::run([]() -> int { throw hhc::RequestError(503, "capture_unavailable", 2); });
	try {
		hhc::workerResult(f);
		++errors;
	} catch (const hhc::RequestError &e) {
		if (e.status != 503 || e.retryAfter != 2)
			++errors;
	} catch (...) {
		++errors;
	}
	QTcpServer server;
	server.listen(QHostAddress::LocalHost, 0);
	std::atomic<bool> cancel = false;
	QElapsedTimer elapsed;
	elapsed.start();
	QFutureWatcher<int> worker;
	QEventLoop loop;
	QObject::connect(&worker, &QFutureWatcher<int>::finished, &loop, &QEventLoop::quit);
	worker.setFuture(QtConcurrent::run([&] {
		hhc::HttpCancellationScope scope(cancel);
		try {
			hhc::httpRequest("GET", QUrl(QString("http://127.0.0.1:%1/stalled").arg(server.serverPort())));
			return 1;
		} catch (const hhc::RequestError &e) {
			return e.code == "transport_cancelled" ? 0 : 2;
		}
	}));
	QTimer::singleShot(150, &app, [&] { cancel = true; });
	QTimer::singleShot(3000, &loop, &QEventLoop::quit);
	loop.exec();
	if (!worker.isFinished() || elapsed.elapsed() > 2000)
		++errors;
	cancel = true;
	worker.waitForFinished();
	if (hhc::workerResult(worker.future()) != 0)
		++errors;
	return errors ? 1 : 0;
}
