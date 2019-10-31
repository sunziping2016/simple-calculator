#define _CRT_SECURE_NO_WARNINGS

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define MAX_BUFFER_LENGTH 100
#define MAX_VARIABLE_NAME 20
#define MAX_VARIABLE_LENGTH 100

#ifndef M_E
#define M_E        2.71828182845904523536   // e
#endif // !M_E
#ifndef M_PI
#define M_PI       3.14159265358979323846   // pi
#endif // !M_PI


char variable_name[MAX_VARIABLE_LENGTH][MAX_VARIABLE_NAME] = { "pi", "e" };
double variable_value[MAX_VARIABLE_LENGTH] = { M_PI, M_E };
size_t variable_size = 2;

char buffer[MAX_BUFFER_LENGTH];
size_t begin, end;

ssize_t getVariable(char* name) {
    for (size_t i = 0; i < variable_size; ++i)
        if (strcmp(name, variable_name[i]) == 0)
            return i;
    return -1;
}

void skipWhitespace() {
    while (begin < end && isspace(buffer[begin]))
        ++begin;
}

/*
 * primary_expression
 *   : identifier
 *   | constant
 *   | '(' assignment_expression ')'
 *   ;
 */
double parseAssignment();
double parsePrimary() {
    skipWhitespace();
    if (begin >= end)
        return 0.0;
    if (buffer[begin] == '(') {
        ++begin;
        double value = parseAssignment();
        skipWhitespace();
        if (begin < end && buffer[begin] == ')') {
            ++begin;
            return value;
        }
        return 0.0;
    }
    if (isalpha(buffer[begin]) || buffer[begin] == '_') {
        size_t origin_begin = begin;
        ++begin;
        while (begin < end && (isalnum(buffer[begin]) || buffer[begin] == '_'))
            ++begin;
        if (begin - origin_begin >= MAX_VARIABLE_NAME)
            return 0.0;
        char name[MAX_VARIABLE_NAME];
        memmove(name, buffer + origin_begin, begin - origin_begin);
        name[begin - origin_begin] = '\0';
		ssize_t index = getVariable(name);
        if (index < 0)
            return 0.0;
        return variable_value[index];
    }
    double value;
    size_t read;
    if (sscanf(buffer + begin, "%lf%ln", &value, &read) <= 0) // NOLINT(cert-err34-c)
        return 0.0;
    begin += read;
    return value;
}

/*
 * postfix_expression
 *   : primary_expression
 *   | 'sum' '(' identifier ',' assignment_expression ',' assignment_expression ',' assignment_expression ')'
 *   | 'cos' '(' assignment_expression ')'
 *   ;
 */

double parsePostfix() {
    skipWhitespace();
    if (begin >= end)
        return 0.0;
    if (isalpha(buffer[begin]) || buffer[begin] == '_') {
        size_t try_begin = begin + 1;
        while (try_begin < end && (isalnum(buffer[try_begin]) || buffer[try_begin] == '_'))
            ++try_begin;
        size_t name_len = try_begin - begin;
        if (name_len == 3) {
            if (memcmp(buffer + begin, "cos", 3) == 0) {
                begin = try_begin;
                skipWhitespace();
                if (buffer[begin] != '(')
                    return 0.0;
                ++begin;
                double value = parseAssignment();
                skipWhitespace();
                if (buffer[begin] != ')')
                    return 0.0;
                ++begin;
                return cos(value);
            }
            if (memcmp(buffer + begin, "sum", 3) == 0) {
                begin = try_begin;
                skipWhitespace();
                if (buffer[begin] != '(')
                    return 0.0;
                ++begin;
                skipWhitespace();
                try_begin = begin;
                if (try_begin < end && (isalpha(buffer[try_begin]) || buffer[try_begin] == '_')) {
                    while (try_begin < end && (isalnum(buffer[try_begin]) || buffer[try_begin] == '_'))
                        ++try_begin;
                }
				ssize_t index = -1;
                if (try_begin - begin > 0) {
                    if (try_begin - begin < MAX_VARIABLE_NAME) {
                        char name[MAX_VARIABLE_NAME];
                        memcpy(name, buffer + begin, try_begin - begin);
                        name[try_begin - begin] = '\0';
                        index = getVariable(name);
                        if (index == -1) {
                            index = variable_size;
                            ++variable_size;
                            strcpy(variable_name[index], name);
                        }
                    }
                }
                begin = try_begin;
                skipWhitespace();
                if (begin >= end || buffer[begin] != ',')
                    return 0.0;
                ++begin;
                double b = parseAssignment();
                skipWhitespace();
                if (begin >= end || buffer[begin] != ',')
                    return 0.0;
                ++begin;
                double e = parseAssignment();
                skipWhitespace();
                if (begin >= end || buffer[begin] != ',')
                    return 0.0;
                ++begin;
                size_t origin_begin = begin;
                double sum = 0;
                for (double i = b; i <= e; i += 1) { // NOLINT(cert-flp30-c)
                    if (index != -1)
                        variable_value[index] = i;
                    sum += parseAssignment();
                    begin = origin_begin;
                }
                parseAssignment();
                skipWhitespace();
                if (begin >= end || buffer[begin] != ')')
                    return 0.0;
                ++begin;
                return sum;
            }
        }
    }
    return parsePrimary();
}

/*
 * unary_expression
 *   : postfix_expression
 *   | '+' unary_expression
 *   | '-' unary_expression
 *   | '!' unary_expression
 *   ;
 */

double parseUnary() {
    skipWhitespace();
    if (begin >= end)
        return 0.0;
    if (buffer[begin] == '+') {
        ++begin;
        return parseUnary();
    }
    else if (buffer[begin] == '-') {
        ++begin;
        return -parseUnary();
    }
    else if (buffer[begin] == '!') {
        ++begin;
        return !parseUnary();
    }
    return parsePostfix();
}

/*
 * multiplicative_expression
 *   : unary_expression (('*'|'/') unary_expression)*
 *   ;
 */

double parseMultiplicative() {
    double value = parseUnary();
    while (1) {
        skipWhitespace();
        if (begin >= end)
            break;
        if (buffer[begin] == '*') {
            ++begin;
            value *= parseUnary();
        }
        else if (buffer[begin] == '/') {
            ++begin;
            value /= parseUnary();
        }
        else
            break;
    }
    return value;
}

/*
 * additive_expression
 *   : multiplicative_expression (('+'|'-') multiplicative_expression)*
 *   ;
 */

double parseAdditive() {
    double value = parseMultiplicative();
    while (1) {
        skipWhitespace();
        if (begin >= end)
            break;
        if (buffer[begin] == '+') {
            ++begin;
            value += parseMultiplicative();
        }
        else if (buffer[begin] == '-') {
            ++begin;
            value -= parseMultiplicative();
        }
        else
            break;
    }
    return value;
}

/*
 * relational_expression
 *   : additive_expression (('<'|'>'|'<='|'>=') additive_expression)*
 *   ;
 */
double parseRelational() {
    double value = parseAdditive();
    while (1) {
        skipWhitespace();
        if (begin >= end)
            break;
        if (begin + 1 < end && buffer[begin] == '<' && buffer[begin + 1] == '=') {
            begin += 2;
            value = value <= parseAdditive();
        }
        else if (begin + 1 < end && buffer[begin] == '>' && buffer[begin + 1] == '=') {
            begin += 2;
            value = value >= parseAdditive();
        }
        else if (buffer[begin] == '<') {
            ++begin;
            value = value < parseAdditive();
        }
        else if (buffer[begin] == '>') {
            ++begin;
            value = value > parseAdditive();
        }
        else
            break;
    }
    return value;
}

/*
 * equality_expression
 *   : relational_expression (('=='|'!=') relational_expression)*
 *   ;
 */

double parseEquality() {
    double value = parseRelational();
    while (1) {
        skipWhitespace();
        if (begin + 1 >= end)
            break;
        if (buffer[begin] == '=' && buffer[begin + 1] == '=') {
            begin += 2;
            value = value == parseRelational();
        }
        else if (buffer[begin] == '!' && buffer[begin + 1] == '=') {
            begin += 2;
            value = value != parseRelational();
        }
        else
            break;
    }
    return value;
}

/*
 * logical_and_expression
 *   : equality_expression ('&&' equality_expression)*
 *   ;
 */

double parseLogicalAnd() {
    double value = parseEquality();
    while (1) {
        skipWhitespace();
        if (begin + 1 >= end)
            break;
        if (buffer[begin] == '&' && buffer[begin + 1] == '&') {
            begin += 2;
            value = value && parseEquality();
        }
        else
            break;
    }
    return value;
}

/*
 * logical_or_expression
 *   : logical_and_expression ('||' logical_and_expression)*
 *   ;
 */

double parseLogicalOr() {
    double value = parseLogicalAnd();
    while (1) {
        skipWhitespace();
        if (begin + 1 >= end)
            break;
        if (buffer[begin] == '|' && buffer[begin + 1] == '|') {
            begin += 2;
            value = value || parseLogicalAnd();
        }
        else
            break;
    }
    return value;
}

/*
 * conditional_expression
 *   : logical_or_expression
 *   | logical_or_expression '?' assignment_expression ':' conditional_expression
 *   ;
 */

double parseConditional() {
    double condition = parseLogicalOr();
    skipWhitespace();
    if (begin < end && buffer[begin] == '?') {
        ++begin;
        double true_value = parseAssignment();
        skipWhitespace();
        if (begin < end && buffer[begin] == ':') {
            ++begin;
            double false_value = parseConditional();
            return condition ? true_value : false_value;
        }
        else
            return 0.0;
    }
    return condition;
}

/*
 * assignment_expression
 *   : conditional_expression
 *   | identifier '=' assignment_expression
 *   ;
 */

double parseAssignment() {
    skipWhitespace();
    size_t try_begin = begin;
    if (try_begin < end && (isalpha(buffer[try_begin]) || buffer[try_begin] == '_')) {
        ++try_begin;
        while (try_begin < end && (isalnum(buffer[try_begin]) || buffer[try_begin] == '_'))
            ++try_begin;
        if (try_begin - begin >= MAX_VARIABLE_NAME)
            return 0.0;
        char name[MAX_VARIABLE_NAME];
        memcpy(name, buffer + begin, try_begin - begin);
        name[try_begin - begin] = '\0';
        while (try_begin < end && isspace(buffer[try_begin]))
            ++try_begin;
        if (try_begin < end && buffer[try_begin] == '=') {
            begin = try_begin + 1;
            ssize_t index = getVariable(name);
            if (index == -1) {
                if (variable_size >= MAX_VARIABLE_LENGTH)
                    return 0.0;
                index = variable_size;
                ++variable_size;
                strcpy(variable_name[index], name);
            }
            variable_value[index] = parseAssignment();
            return variable_value[index];
        }
    }
    return parseConditional();
}

int main() {
    while (1) {
        fgets(buffer, MAX_BUFFER_LENGTH, stdin);
        begin = 0;
        end = strlen(buffer);
        if (end > 1) {
            if (buffer[end - 1] != '\n') {
                fprintf(stderr, "Buffer overflow.\n");
                break;
            }
            else {
                --end;
            }
        }
        double result = parseAssignment();
        printf("begin: %lu end: %lu\n", begin, end);
        printf("result: %lf\n", result);
    }
    return 0;
}