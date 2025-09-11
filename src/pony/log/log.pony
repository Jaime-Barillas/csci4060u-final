"""
# Asynchronous Logging for Pony

There are three kinds of objects required for logging:
1. `Logger`s.
2. `LogTarget`s.
3. Log messages.

## Logger
A `Logger` has a name, `LogTarget`, log level (defaults to `Info`), and
is responsible for queueing log messages to the specified `LogTarget`. Log
messages are queued to the `LogTarget` by calling one of the `info`, `warn`,
or `err` functions.

Log messages are queued only when all of the following are true:
1. The `-DLogEnabled` build flag is defined.
2. The log level of the message is greater than, or equal to, the log level
   of the `Logger`.

## LogTarget

The log message, along with the `Logger`'s name and log level, are passed
unprocessed to the `LogTarget`. The `LogTarget` will then, asynchronously
format the log message and perform the actual logging.

`LogTarget`'s are implemented as actors, and since they ultimately recieve
the log message, all log messages passed to `Logger`s must have the `val`
reference capability.

## Log Messages

A log "message" is any object that implements the `Stringable` interface.
Additionally, it must have the `val` reference capability. Conversion from an
object to a loggable string happens asynchronously when a `LogTarget` calls
the object's `string` function.

## Examples
### Logging Strings
```
use "log"

actor Main
  new create(env: Env) =>
    let target = ConsoleTarget(env.out)
    let logger = Logger("logger", target, Warn)
    logger.info("info log") // Not printed, info log level too low.
    logger.warn("warn log") // Prints "[logger] Warn: warn log" to std.out.
    logger.err("error log") // Prints "[logger] Error: error log" to std.out.
```

### Logging Custom Objects
```
use "log"

class val Dog
  let _name: String

  new val create(name: String) =>
    _name = name

  fun string(): String iso^ => _name + ": Bow Wow"

actor Main
  new create(env: Env) =>
    let target = ConsoleTarget(env.out)
    let logger = Logger("logger", target)
    let doggo = Dog("Scruffy")
    logger.info[Dog](doggo)
```

## TODO

* Source location info.
* String interpolation for log messages?
* Log formatters?
"""

use "term"

primitive Info
  fun apply(): U64 => 0
  fun string(): String => "Info"

primitive Warn
  fun apply(): U64 => 1
  fun string(): String => "Warn"

primitive Error
  fun apply(): U64 => 2
  fun string(): String => "Error"

type LogLevel is (Info | Warn | Error)


/*=========*/
/* Sources */
/*=========*/
class Logger
  let _name: String
  let _target: LogTarget
  let _log_level: LogLevel

  new create(name: String, target: LogTarget, log_level: LogLevel = Info) =>
    _name = name
    _target = target
    _log_level = log_level

  fun _log[T: Stringable val](obj: T, log_level: LogLevel) =>
    if log_level() >= _log_level() then
      _target.log[T](_name, log_level, obj)
    end

  fun info[T: Stringable val = String](obj: T) =>
    ifdef "LogEnabled" then
      _log[T](obj, Info)
    end

  fun warn[T: Stringable val = String](obj: T) =>
    ifdef "LogEnabled" then
      _log[T](obj, Warn)
    end

  fun err[T: Stringable val = String](obj: T) =>
    ifdef "LogEnabled" then
      _log[T](obj, Error)
    end


/*=========*/
/* Targets */
/*=========*/
trait tag LogTarget
  fun _out(): OutStream
  be log[T: Stringable val](name: String, log_level: LogLevel, obj: T)

actor ConsoleTarget is LogTarget
  let _out_stream: OutStream

  new create(out: OutStream) =>
    _out_stream = out

  fun _out(): OutStream => _out_stream

  be log[T: Stringable val](name: String, log_level: LogLevel, obj: T) =>
    let log_level_str = log_level.string()
    let log_level_colour_str = match log_level
    | let _: Info  => ANSI.bright_cyan()
    | let _: Warn  => ANSI.bright_yellow()
    | let _: Error => ANSI.bright_red()
    end

    let obj_str = match obj
    | let s: String => s // Avoid copying when obj is String
    else
      obj.string()
    end

    let str: String iso = String(
      4 // '[', ']' & ': '
      + ANSI.bold().size()
      + log_level_colour_str.size()
      + (2 * ANSI.reset().size())
      + name.size()
      + log_level_str.size()
      + obj_str.size()
    )

    str.append(ANSI.bold())
    str.append("["); str.append(name); str.append("] ")
    str.append(ANSI.reset())
    str.append(log_level_colour_str); str.append(log_level_str)
    str.append(ANSI.reset())
    str.append(": ")
    str.append(obj_str)
    _out().print(consume str)
