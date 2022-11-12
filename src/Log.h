#pragma once
#ifndef LOG_H_
#define LOG_H_

void Log_global_init(void);
void Debug(const char *fmt, ...);
void Info(const char *fmt, ...);
void Warn(const char *fmt, ...);
void Panic(const char *fmt, ...);

#endif /* LOG_H_ */

