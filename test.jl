module test

import io
import async



do_something = (c) async -> c.complete('Hello, world!')
completer = async.Completer()
do_something(completer)
io.println(await completer.as_future())