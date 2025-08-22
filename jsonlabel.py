import json
from datetime import datetime, timedelta, time

# -------- CONFIG --------
input_file = "c:/Users/X415/Downloads/sensor_log_raw.json"
output_file = "c:/Users/X415/Downloads/sensor_log_new.json"

DATE_FORMAT = "%m-%d %H:%M"
YEAR = 2025

# Base label ranges
label_ranges = [
    (datetime(YEAR, 6, 19, 0, 0), "Overdried"),
    (datetime(YEAR, 7, 18, 0, 0), "Shocked"),
    (datetime(YEAR, 7, 23, 6, 40), "Overwatered"),
    (datetime(YEAR, 7, 30, 0, 0), "Lightly underwatered"),
]

# Special override periods
override_healthy_1_start = datetime(YEAR, 7, 14, 0, 0)
override_healthy_1_end   = datetime(YEAR, 7, 15, 23, 59)

# For splitting "Overwatered"
overwatered_start = datetime(YEAR, 7, 23, 6, 40)
overwatered_end   = datetime(YEAR, 7, 30, 0, 0)
overwatered_duration = overwatered_end - overwatered_start
overwatered_split = overwatered_start + timedelta(seconds=overwatered_duration.total_seconds() * (2/3))

# Nighttime filter
night_start = time(22, 0)
night_end = time(5, 30)

# -------- FUNCTIONS --------
def is_night(ts: datetime) -> bool:
    """Return True if timestamp is between 22:00 and 05:30."""
    return ts.time() >= night_start or ts.time() <= night_end

def get_label(ts: datetime) -> str:
    """Return label based on timestamp with overrides."""
    
    # Override 1: 14–15 July
    if override_healthy_1_start <= ts <= override_healthy_1_end:
        return "Healthy"
    
    # Override 2: first 2/3 of Overwatered
    if overwatered_start <= ts < overwatered_split:
        return "Healthy"
    
    # Normal labeling logic
    current_label = "Healthy"
    for change_time, label in label_ranges:
        if ts >= change_time:
            current_label = label
        else:
            break
    return current_label

# -------- MAIN --------
with open(input_file, "r") as f:
    data = json.load(f)

filtered = []
for entry in data:
    ts = datetime.strptime(entry["timestamp"], DATE_FORMAT).replace(year=YEAR)
    if not is_night(ts):
        entry["label"] = get_label(ts)
        filtered.append(entry)

with open(output_file, "w") as f:
    json.dump(filtered, f, indent=2)

print(f"Filtered data saved to {output_file}. {len(filtered)} entries remain.")
