#pragma once
#include <QFuture>
#include <QException>
#include <exception>
namespace hhc {
template<class T> T workerResult(const QFuture<T> &future)
{
	try {
		return future.result();
	} catch (const QUnhandledException &e) {
		std::rethrow_exception(e.exception());
	}
}
} // namespace hhc
