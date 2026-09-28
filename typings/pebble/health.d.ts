/*
* Copyright (c) 2026 Moddable Tech, Inc.
*
*   This file is part of the Moddable SDK Tools.
*
*   The Moddable SDK Tools is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   The Moddable SDK Tools is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
*
*/

type HealthMetricName =
  | "step count"
  | "active seconds"
  | "walked distance"
  | "sleep seconds"
  | "sleep restful seconds"
  | "resting calories"
  | "active calories"
  | "heart rate"
  | "heart rate raw";

type HealthAggregation = "sum" | "average" | "min" | "max";

type HealthScope = "once" | "weekly" | "weekday or weekend" | "daily";

type HealthIterationDirection = "past" | "future";

type HealthMeasurementSystem = "metric" | "imperial" | undefined;

interface HealthMetricQueryOptions {
  metric: HealthMetricName;
  start?: Date | number;
  end?: Date | number;
  aggregation?: HealthAggregation;
  scope?: HealthScope;
}

interface HealthMetricAccessibleOptions {
  metric: HealthMetricName;
  start: Date | number;
  end: Date | number;
  aggregation?: HealthAggregation;
  scope?: HealthScope;
}

type HealthActivity = typeof Health.activity[
  "sleep" | "restfulSleep" | "walk" | "run" | "openWorkout"
];

interface HealthActivityIterateOptions {
  activities: number;
  start: Date | number;
  end: Date | number;
  direction?: HealthIterationDirection;
  callback: (activity: HealthActivity, start: number, end: number) => boolean;
}

interface HealthActivityAccessibleOptions {
  activities: number;
  start: Date | number;
  end: Date | number;
}

interface HealthMetricAlertOptions {
  metric: HealthMetricName;
  threshold: number;
}

declare class HealthMetricAlert implements Disposable {
  constructor(options: HealthMetricAlertOptions);
  close(): void;
  [Symbol.dispose](): void;
}

interface HealthMinuteHistoryOptions {
  length: number;
  start: Date | number;
  end: Date | number;
}

interface HealthMinuteRecord {
  steps: number;
  orientation: number;
  vmc: number;
  light: number;
  heartRate: number;
}

interface HealthMinuteHistory extends Array<HealthMinuteRecord | undefined> {
  start: number;
  end: number;
}

declare class Health {
  static metric: {
    get(name: HealthMetricName): number;
    query(options: HealthMetricQueryOptions): number;
    accessible(options: HealthMetricAccessibleOptions): number;
    readonly Alert: typeof HealthMetricAlert;
  };

  static activity: {
    get(): number;
    iterate(options: HealthActivityIterateOptions): void;
    accessible(options: HealthActivityAccessibleOptions): number;

    readonly sleep: 1;
    readonly restfulSleep: 2;
    readonly walk: 4;
    readonly run: 8;
    readonly openWorkout: 16;
  };

  static heartRate: {
    /** Write-only. Assign desired sample period in milliseconds; assign 0 to restore automatic sampling. */
    samplePeriod: number;
    readonly samplePeriodExpiration: number;
  };

  static history: {
    byMinute(options: HealthMinuteHistoryOptions): HealthMinuteHistory;
  };

  static access: {
    readonly available: 1;
    readonly permission: 2;
    readonly supported: 4;
    readonly data: 8;
  };

  static displayMeasurementSystem(metric: HealthMetricName): HealthMeasurementSystem;
}

export default Health;
