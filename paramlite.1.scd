PARAMLITE(1)

# NAME

paramlite - Execute SQLite prepared statements in the shell

# SYNOPSIS

*paramlite* [_OPTIONS_...] [_ORDERED PARAMETERS_...] _DATABASE_... < _QUERY_

# DESCRIPTION

*paramlite* executes SQLite prepared statements in the command line,
without having to interface with a programming language. The program
reads a query from the standard input and replaces any placeholders
(literal question marks, i.e. _?_) with the corresponding parameters
provided as command line arguments in order. This prevents possible
SQL injection attacks and provides simple integration with other
command-line tools.

# OPTIONS

## Parameters

*-n*++
	Represents a NULL value for parameter substitution.

*-d* _NUMBER_++
	Specify a 64-bit integer parameter.

*-f* _NUMBER_++
	Specify a double-precision floating-point number parameter.

*-t* _TEXT_++
	Specify a text parameter.

## Output options

*-F* _SEPARATOR_++
	Specify the output field separator (default: "\\t", a tab).

*-R* _SEPARATOR_++
	Specify the output record separator (default: "\\n", a newline).

*-N* _STRING_++
	Specify a stand-in string for NULL values (default: "[NULL]").

*-B* _STRING_++
	Specify a stand-in string for BLOB values.

## Database options

*-r*++
	Open the database for read-only access (default).

*-w*++
	Open the database for writing.

*-c*++
	Create the database if it does not exist; implies -w.

## Other options

*-v*++
	Display version information.

*-h*++
	Show help and usage information.

# EXIT CODES
*0*	Success.

*1*	SQLite error, like a syntax error in the query.

*2*	Invalid command-line arguments, or no database selected.

# EXAMPLES

Basic query execution:
```
echo "SELECT * FROM table WHERE id = ?;" |
paramlite -d 42 database.db
```

Query with multiple parameters:
```
echo "INSERT INTO table (name, age) VALUES (?, ?);" |
paramlite -t "Alice" -d 30 -w database.db
```

Custom output formatting:
```
echo "SELECT * FROM table;" |
paramlite -F ',' -R '\\n\\n' database.db
```

# BUGS

Most likely. If you find a bug, submit an issue to the project
repository at _https://github.com/hlischt/paramlite_.

# SEE ALSO
sqlite(1)++
https://www.sqlite.org/lang_expr.html#varparam


