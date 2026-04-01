#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static bool print(const char* data, size_t length) {
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
		if (putchar(bytes[i]) == EOF)
			return false;
	return true;
}

int printf(const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);

	int written = 0;

	while (*format != '\0') {
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;

		switch (*format) {

			case 'c': {
				format++;
				char c = (char) va_arg(parameters, int /* char promotes to int */);
				if (!maxrem) {
					// TODO: Set errno to EOVERFLOW.
					return -1;
				}
				if (!print(&c, sizeof(c)))
					return -1;
				written++;
				break;
			}

			case 'd': {
				format++;
				int d = va_arg(parameters, int /* char promotes to int */);
				bool has_a_sign = d < 0;
				if (d < 0) d = - d;
				char buffer[16] = {'\0'};
				int i = 0;
				if (d == 0) buffer[i++] = '0';
				while (d > 0) {
					char c = (d % 10) + '0';
					d = d / 10;
					buffer[i++] = c;
				}
				if (has_a_sign) {
					buffer[i++] = '-';
				}
				buffer[i] = '\0';
				size_t len = i;
					
				for (int j = 0; 2*j < i; j++) {
					char rem = buffer[j];
					int a = i - 1 - j;
					buffer[j] = buffer[a];
					buffer[a] = rem;
				}
				
				if (maxrem < len) {
					// TODO: Set errno to EOVERFLOW.
					return -1;
				}
				if (!print(buffer, len))
					return -1;
				written += len;
				break;
			}

			case 'u': {
				format++;
				unsigned int d = va_arg(parameters, unsigned int /* char promotes to int */);
				char buffer[16] = {'\0'};
				int i = 0;
				if (d == 0) buffer[i++] = '0';
				while (d > 0) {
					char c = (d % 10) + '0';
					d = d / 10;
					buffer[i++] = c;
				}
				buffer[i] = '\0';
				size_t len = i;
					
				for (int j = 0; 2*j < i; j++) {
					char rem = buffer[j];
					int a = i - 1 - j;
					buffer[j] = buffer[a];
					buffer[a] = rem;
				}
				
				if (maxrem < len) {
					// TODO: Set errno to EOVERFLOW.
					return -1;
				}
				if (!print(buffer, len))
					return -1;
				written += len;
				break;
			}

			case 'x': {
				format++;
				int d = va_arg(parameters, int /* char promotes to int */);
				bool has_a_sign = d < 0;
				if (d < 0) d = - d;
				char buffer[16] = {'\0'};
				int i = 0;
				if (d == 0) buffer[i++] = '0';
				while (d > 0) {
					char c = (d % 16) + '0';
					if (c > '9') c += 'A' - '9' - 1;
					d = d / 16;
					buffer[i++] = c;
				}
				if (has_a_sign) {
					buffer[i++] = '-';
				}
				buffer[i++] = 'x';
				buffer[i++] = '0';
				buffer[i] = '\0';
				size_t len = i;
					
				for (int j = 0; 2*j < i; j++) {
					char rem = buffer[j];
					int a = i - 1 - j;
					buffer[j] = buffer[a];
					buffer[a] = rem;
				}
				
				if (maxrem < len) {
					// TODO: Set errno to EOVERFLOW.
					return -1;
				}
				if (!print(buffer, len))
					return -1;
				written += len;
				break;
			}

			case 's': {
				format++;
				const char* str = va_arg(parameters, const char*);
				size_t len = strlen(str);
				if (maxrem < len) {
					// TODO: Set errno to EOVERFLOW.
					return -1;
				}
				if (!print(str, len))
					return -1;
				written += len;
				break;
			}

			default : {
				format = format_begun_at;
				size_t len = strlen(format);
				if (maxrem < len) {
					// TODO: Set errno to EOVERFLOW.
					return -1;
				}
				if (!print(format, len))
					return -1;
				written += len;
				format += len;
			}
		}
	}

	va_end(parameters);
	return written;
}
