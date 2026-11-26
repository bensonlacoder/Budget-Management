#pragma once

#include "budget.h"

#include <QString>

// Reads dining habits from a CSV file's text (T07, SP-08). The format:
//
//   day,meals
//   Monday,2
//   Wednesday,3
//
// The header line is optional. Days may be full names or three-letter abbreviations
// in any case, and days not listed have 0 meals. Blank lines are ignored.
namespace HabitsCsv {

// Files larger than this are rejected without being read.
constexpr qint64 MaxFileBytes = 10 * 1024;

// Returns true and fills `habits` (Monday first) if the whole text is valid.
// Otherwise returns false, leaves `habits` unchanged, and sets `error` to a message
// naming the first bad line.
bool parse(const QString &text, Budget::DiningHabits &habits, QString &error);

} // namespace HabitsCsv
