# import matplotlib.pyplot as plt
# import datetime

# # Raw log data: (timestamp, max_value)
# raw_data = [
#     ("00:00:19.930", 952),
#     ("00:00:39.953", 969),
#     ("00:00:59.973", 941),
#     ("00:01:19.993", 946),
#     ("00:01:40.013", 986),
#     ("00:02:00.033", 901),
#     ("00:02:20.054", 910),
#     ("00:02:40.074", 914),
#     ("00:03:00.094", 1017),
#     ("00:03:20.115", 925),
#     ("00:03:40.136", 974),
#     ("00:04:00.156", 907),
#     ("00:04:20.177", 965),
#     ("00:04:40.202", 901),
#     ("00:05:00.222", 974),
#     ("00:05:20.242", 904),
#     ("00:05:40.262", 1040),
#     ("00:06:00.282", 942),
#     ("00:06:20.303", 903),
#     ("00:06:40.323", 923)
# ]

# # Convert timestamps to seconds
# def time_to_seconds(time_str):
#     dt = datetime.datetime.strptime(time_str, "%H:%M:%S.%f")
#     return dt.minute * 60 + dt.second + dt.microsecond / 1e6

# times = [time_to_seconds(t) for t, _ in raw_data]
# values = [v for _, v in raw_data]

# # Plot
# plt.figure(figsize=(10, 5))
# plt.plot(times, values, marker='o', linestyle='-', color='teal', label="Max measured value")
# plt.axhline(y=1040, color='red', linestyle='--', label="Wet threshold")

# plt.title("Max Sensor Value Over Time")
# plt.xlabel("Time (seconds)")
# plt.ylabel("Max Sensor Value")
# plt.grid(True)
# plt.legend()
# plt.tight_layout()
# plt.savefig("./plot1.png")  # save the plot


import matplotlib.pyplot as plt
import datetime
import re

log = """
[00:00:00.004,760] <inf> Plant_sensor: measured data: 874 mV
[00:00:00.004,760] <inf> Plant_sensor: smooth data: 54 mV
[00:00:00.104,888] <inf> Plant_sensor: measured data: 874 mV
[00:00:00.104,888] <inf> Plant_sensor: smooth data: 105 mV
[00:00:00.205,017] <inf> Plant_sensor: measured data: 878 mV
[00:00:00.205,047] <inf> Plant_sensor: smooth data: 154 mV
[00:00:00.305,145] <inf> Plant_sensor: measured data: 875 mV
[00:00:00.305,175] <inf> Plant_sensor: smooth data: 199 mV
[00:00:00.405,273] <inf> Plant_sensor: measured data: 870 mV
[00:00:00.405,303] <inf> Plant_sensor: smooth data: 241 mV
[00:00:00.505,401] <inf> Plant_sensor: measured data: 874 mV
[00:00:00.505,432] <inf> Plant_sensor: smooth data: 280 mV
[00:00:00.605,529] <inf> Plant_sensor: measured data: 878 mV
[00:00:00.605,560] <inf> Plant_sensor: smooth data: 317 mV
[00:00:00.705,657] <inf> Plant_sensor: measured data: 872 mV
[00:00:00.705,688] <inf> Plant_sensor: smooth data: 352 mV
[00:00:00.805,786] <inf> Plant_sensor: measured data: 875 mV
[00:00:00.805,816] <inf> Plant_sensor: smooth data: 385 mV
[00:00:00.905,944] <inf> Plant_sensor: measured data: 872 mV
[00:00:00.905,944] <inf> Plant_sensor: smooth data: 415 mV
[00:00:01.006,042] <inf> Plant_sensor: measured data: 875 mV
[00:00:01.006,072] <inf> Plant_sensor: smooth data: 444 mV
[00:00:01.106,170] <inf> Plant_sensor: measured data: 869 mV
[00:00:01.106,201] <inf> Plant_sensor: smooth data: 470 mV
[00:00:01.206,298] <inf> Plant_sensor: measured data: 867 mV
[00:00:01.206,329] <inf> Plant_sensor: smooth data: 495 mV
[00:00:01.306,427] <inf> Plant_sensor: measured data: 871 mV
[00:00:01.306,457] <inf> Plant_sensor: smooth data: 519 mV
[00:00:01.406,555] <inf> Plant_sensor: measured data: 871 mV
[00:00:01.406,585] <inf> Plant_sensor: smooth data: 541 mV
[00:00:01.506,683] <inf> Plant_sensor: measured data: 873 mV
[00:00:01.506,713] <inf> Plant_sensor: smooth data: 561 mV
[00:00:01.606,811] <inf> Plant_sensor: measured data: 873 mV
[00:00:01.606,842] <inf> Plant_sensor: smooth data: 581 mV
[00:00:01.706,939] <inf> Plant_sensor: measured data: 870 mV
[00:00:01.706,970] <inf> Plant_sensor: smooth data: 599 mV
[00:00:01.807,067] <inf> Plant_sensor: measured data: 873 mV
[00:00:01.807,098] <inf> Plant_sensor: smooth data: 616 mV
[00:00:01.907,196] <inf> Plant_sensor: measured data: 873 mV
[00:00:01.907,226] <inf> Plant_sensor: smooth data: 632 mV
[00:00:02.007,324] <inf> Plant_sensor: measured data: 873 mV
[00:00:02.007,354] <inf> Plant_sensor: smooth data: 647 mV
[00:00:02.107,452] <inf> Plant_sensor: measured data: 872 mV
[00:00:02.107,482] <inf> Plant_sensor: smooth data: 661 mV
[00:00:02.207,580] <inf> Plant_sensor: measured data: 870 mV
[00:00:02.207,611] <inf> Plant_sensor: smooth data: 674 mV
[00:00:02.307,708] <inf> Plant_sensor: measured data: 873 mV
[00:00:02.307,739] <inf> Plant_sensor: smooth data: 686 mV
[00:00:02.407,836] <inf> Plant_sensor: measured data: 873 mV
[00:00:02.407,867] <inf> Plant_sensor: smooth data: 698 mV
[00:00:02.507,965] <inf> Plant_sensor: measured data: 872 mV
[00:00:02.507,995] <inf> Plant_sensor: smooth data: 709 mV
[00:00:02.608,093] <inf> Plant_sensor: measured data: 874 mV
[00:00:02.608,123] <inf> Plant_sensor: smooth data: 719 mV
[00:00:02.708,221] <inf> Plant_sensor: measured data: 873 mV
[00:00:02.708,251] <inf> Plant_sensor: smooth data: 729 mV
[00:00:02.808,349] <inf> Plant_sensor: measured data: 871 mV
[00:00:02.808,380] <inf> Plant_sensor: smooth data: 738 mV
[00:00:02.908,477] <inf> Plant_sensor: measured data: 871 mV
[00:00:02.908,508] <inf> Plant_sensor: smooth data: 746 mV
[00:00:03.008,605] <inf> Plant_sensor: measured data: 870 mV
[00:00:03.008,636] <inf> Plant_sensor: smooth data: 754 mV
[00:00:03.108,734] <inf> Plant_sensor: measured data: 871 mV
[00:00:03.108,764] <inf> Plant_sensor: smooth data: 761 mV
[00:00:03.208,862] <inf> Plant_sensor: measured data: 873 mV
[00:00:03.208,892] <inf> Plant_sensor: smooth data: 768 mV
[00:00:03.308,990] <inf> Plant_sensor: measured data: 875 mV
[00:00:03.309,020] <inf> Plant_sensor: smooth data: 775 mV
[00:00:03.409,118] <inf> Plant_sensor: measured data: 872 mV
[00:00:03.409,149] <inf> Plant_sensor: smooth data: 781 mV
[00:00:03.509,246] <inf> Plant_sensor: measured data: 872 mV
[00:00:03.509,277] <inf> Plant_sensor: smooth data: 786 mV
[00:00:03.609,375] <inf> Plant_sensor: measured data: 872 mV
[00:00:03.609,405] <inf> Plant_sensor: smooth data: 792 mV
[00:00:03.709,503] <inf> Plant_sensor: measured data: 874 mV
[00:00:03.709,533] <inf> Plant_sensor: smooth data: 797 mV
[00:00:03.809,631] <inf> Plant_sensor: measured data: 874 mV
[00:00:03.809,661] <inf> Plant_sensor: smooth data: 802 mV
[00:00:03.909,759] <inf> Plant_sensor: measured data: 870 mV
[00:00:03.909,790] <inf> Plant_sensor: smooth data: 806 mV
[00:00:04.009,887] <inf> Plant_sensor: measured data: 874 mV
[00:00:04.009,918] <inf> Plant_sensor: smooth data: 810 mV
[00:00:04.110,015] <inf> Plant_sensor: measured data: 872 mV
[00:00:04.110,046] <inf> Plant_sensor: smooth data: 814 mV
[00:00:04.210,144] <inf> Plant_sensor: measured data: 873 mV
[00:00:04.210,174] <inf> Plant_sensor: smooth data: 818 mV
[00:00:04.310,272] <inf> Plant_sensor: measured data: 873 mV
[00:00:04.310,302] <inf> Plant_sensor: smooth data: 821 mV
[00:00:04.410,400] <inf> Plant_sensor: measured data: 873 mV
[00:00:04.410,430] <inf> Plant_sensor: smooth data: 824 mV
[00:00:04.510,528] <inf> Plant_sensor: measured data: 873 mV
[00:00:04.510,559] <inf> Plant_sensor: smooth data: 827 mV
[00:00:04.610,656] <inf> Plant_sensor: measured data: 871 mV
[00:00:04.610,687] <inf> Plant_sensor: smooth data: 830 mV
[00:00:04.710,784] <inf> Plant_sensor: measured data: 875 mV
[00:00:04.710,815] <inf> Plant_sensor: smooth data: 833 mV
[00:00:04.810,913] <inf> Plant_sensor: measured data: 875 mV
[00:00:04.810,943] <inf> Plant_sensor: smooth data: 835 mV
[00:00:04.911,041] <inf> Plant_sensor: measured data: 873 mV
[00:00:04.911,071] <inf> Plant_sensor: smooth data: 838 mV
[00:00:05.011,169] <inf> Plant_sensor: measured data: 870 mV
[00:00:05.011,199] <inf> Plant_sensor: smooth data: 840 mV
[00:00:05.111,297] <inf> Plant_sensor: measured data: 873 mV
[00:00:05.111,328] <inf> Plant_sensor: smooth data: 842 mV
[00:00:05.211,425] <inf> Plant_sensor: measured data: 875 mV
[00:00:05.211,456] <inf> Plant_sensor: smooth data: 844 mV
[00:00:05.311,553] <inf> Plant_sensor: measured data: 871 mV
[00:00:05.311,584] <inf> Plant_sensor: smooth data: 845 mV
[00:00:05.411,682] <inf> Plant_sensor: measured data: 870 mV
[00:00:05.411,712] <inf> Plant_sensor: smooth data: 847 mV
[00:00:05.511,810] <inf> Plant_sensor: measured data: 873 mV
[00:00:05.511,840] <inf> Plant_sensor: smooth data: 848 mV
[00:00:05.611,938] <inf> Plant_sensor: measured data: 873 mV
[00:00:05.611,968] <inf> Plant_sensor: smooth data: 850 mV
[00:00:05.712,066] <inf> Plant_sensor: measured data: 875 mV
[00:00:05.712,097] <inf> Plant_sensor: smooth data: 852 mV
[00:00:05.812,194] <inf> Plant_sensor: measured data: 877 mV
[00:00:05.812,225] <inf> Plant_sensor: smooth data: 853 mV
[00:00:05.912,322] <inf> Plant_sensor: measured data: 878 mV
[00:00:05.912,353] <inf> Plant_sensor: smooth data: 855 mV
[00:00:06.012,451] <inf> Plant_sensor: measured data: 878 mV
[00:00:06.012,481] <inf> Plant_sensor: smooth data: 856 mV
[00:00:06.112,579] <inf> Plant_sensor: measured data: 872 mV
[00:00:06.112,609] <inf> Plant_sensor: smooth data: 857 mV
[00:00:06.212,707] <inf> Plant_sensor: measured data: 872 mV
[00:00:06.212,738] <inf> Plant_sensor: smooth data: 858 mV
[00:00:06.312,835] <inf> Plant_sensor: measured data: 875 mV
[00:00:06.312,866] <inf> Plant_sensor: smooth data: 859 mV
[00:00:06.412,963] <inf> Plant_sensor: measured data: 873 mV
[00:00:06.412,994] <inf> Plant_sensor: smooth data: 860 mV
[00:00:06.513,092] <inf> Plant_sensor: measured data: 873 mV
[00:00:06.513,122] <inf> Plant_sensor: smooth data: 861 mV
[00:00:06.613,220] <inf> Plant_sensor: measured data: 873 mV
[00:00:06.613,250] <inf> Plant_sensor: smooth data: 861 mV
[00:00:06.713,348] <inf> Plant_sensor: measured data: 869 mV
[00:00:06.713,378] <inf> Plant_sensor: smooth data: 862 mV
[00:00:06.813,476] <inf> Plant_sensor: measured data: 870 mV
[00:00:06.813,507] <inf> Plant_sensor: smooth data: 862 mV
[00:00:06.913,604] <inf> Plant_sensor: measured data: 869 mV
[00:00:06.913,635] <inf> Plant_sensor: smooth data: 863 mV
[00:00:07.013,732] <inf> Plant_sensor: measured data: 878 mV
[00:00:07.013,763] <inf> Plant_sensor: smooth data: 864 mV
[00:00:07.113,861] <inf> Plant_sensor: measured data: 872 mV
[00:00:07.113,891] <inf> Plant_sensor: smooth data: 864 mV
[00:00:07.213,989] <inf> Plant_sensor: measured data: 869 mV
[00:00:07.214,019] <inf> Plant_sensor: smooth data: 864 mV
[00:00:07.314,117] <inf> Plant_sensor: measured data: 873 mV
[00:00:07.314,147] <inf> Plant_sensor: smooth data: 865 mV
[00:00:07.414,245] <inf> Plant_sensor: measured data: 870 mV
[00:00:07.414,276] <inf> Plant_sensor: smooth data: 865 mV
[00:00:07.514,373] <inf> Plant_sensor: measured data: 873 mV
[00:00:07.514,404] <inf> Plant_sensor: smooth data: 866 mV
[00:00:07.614,501] <inf> Plant_sensor: measured data: 869 mV
[00:00:07.614,532] <inf> Plant_sensor: smooth data: 866 mV
[00:00:07.714,630] <inf> Plant_sensor: measured data: 874 mV
[00:00:07.714,660] <inf> Plant_sensor: smooth data: 866 mV
[00:00:07.814,758] <inf> Plant_sensor: measured data: 875 mV
[00:00:07.814,788] <inf> Plant_sensor: smooth data: 867 mV
[00:00:07.914,886] <inf> Plant_sensor: measured data: 866 mV
[00:00:07.914,916] <inf> Plant_sensor: smooth data: 867 mV
[00:00:08.015,014] <inf> Plant_sensor: measured data: 878 mV
[00:00:08.015,045] <inf> Plant_sensor: smooth data: 867 mV
[00:00:08.115,142] <inf> Plant_sensor: measured data: 872 mV
[00:00:08.115,173] <inf> Plant_sensor: smooth data: 868 mV
[00:00:08.215,270] <inf> Plant_sensor: measured data: 872 mV
[00:00:08.215,301] <inf> Plant_sensor: smooth data: 868 mV
[00:00:08.315,399] <inf> Plant_sensor: measured data: 874 mV
[00:00:08.315,429] <inf> Plant_sensor: smooth data: 868 mV
[00:00:08.415,527] <inf> Plant_sensor: measured data: 877 mV
[00:00:08.415,557] <inf> Plant_sensor: smooth data: 869 mV
[00:00:08.515,655] <inf> Plant_sensor: measured data: 872 mV
[00:00:08.515,686] <inf> Plant_sensor: smooth data: 869 mV
[00:00:08.615,783] <inf> Plant_sensor: measured data: 870 mV
[00:00:08.615,814] <inf> Plant_sensor: smooth data: 869 mV
[00:00:08.715,911] <inf> Plant_sensor: measured data: 873 mV
[00:00:08.715,942] <inf> Plant_sensor: smooth data: 869 mV
[00:00:08.816,040] <inf> Plant_sensor: measured data: 872 mV
[00:00:08.816,070] <inf> Plant_sensor: smooth data: 869 mV
[00:00:08.916,168] <inf> Plant_sensor: measured data: 873 mV
[00:00:08.916,198] <inf> Plant_sensor: smooth data: 870 mV
[00:00:09.016,296] <inf> Plant_sensor: measured data: 872 mV
[00:00:09.016,326] <inf> Plant_sensor: smooth data: 870 mV
[00:00:09.116,424] <inf> Plant_sensor: measured data: 869 mV
[00:00:09.116,455] <inf> Plant_sensor: smooth data: 870 mV
[00:00:09.216,552] <inf> Plant_sensor: measured data: 869 mV
[00:00:09.216,583] <inf> Plant_sensor: smooth data: 869 mV
[00:00:09.316,680] <inf> Plant_sensor: measured data: 872 mV
[00:00:09.316,711] <inf> Plant_sensor: smooth data: 870 mV
[00:00:09.416,809] <inf> Plant_sensor: measured data: 872 mV
[00:00:09.416,839] <inf> Plant_sensor: smooth data: 870 mV
[00:00:09.516,937] <inf> Plant_sensor: measured data: 873 mV
[00:00:09.516,967] <inf> Plant_sensor: smooth data: 870 mV
[00:00:09.617,065] <inf> Plant_sensor: measured data: 873 mV
[00:00:09.617,095] <inf> Plant_sensor: smooth data: 870 mV
[00:00:09.717,193] <inf> Plant_sensor: measured data: 875 mV
[00:00:09.717,224] <inf> Plant_sensor: smooth data: 870 mV
[00:00:09.817,321] <inf> Plant_sensor: measured data: 873 mV
[00:00:09.817,352] <inf> Plant_sensor: smooth data: 870 mV
[00:00:09.917,449] <inf> Plant_sensor: measured data: 872 mV
[00:00:09.917,480] <inf> Plant_sensor: smooth data: 871 mV
"""

def parse_log(log_text):
    measured = []
    smooth = []
    timestamps = []

    # Regex to capture timestamp, type, and value
    pattern = re.compile(r"\[(\d{2}:\d{2}:\d{2}\.\d{3}),\d+\] <inf> Plant_sensor: (measured|smooth) data: (\d+) mV")

    for line in log_text.splitlines():
        m = pattern.search(line)
        if m:
            timestamp, data_type, value = m.groups()
            value = int(value)
            # convert timestamp to seconds
            dt = datetime.datetime.strptime(timestamp, "%H:%M:%S.%f")
            seconds = dt.minute * 60 + dt.second + dt.microsecond / 1e6
            timestamps.append(seconds)
            if data_type == "measured":
                measured.append(value)
            else:
                smooth.append(value)

    # The log alternates measured and smooth data,
    # so timestamps correspond to both; we'll keep only unique for plot:
    times_unique = timestamps[::2]

    return times_unique, measured, smooth

times, measured_values, smooth_values = parse_log(log)

# Plotting
plt.figure(figsize=(10,6))
plt.plot(times, measured_values, label="Measured data", marker='o')
plt.plot(times, smooth_values, label="Smooth data", marker='x')
plt.xlabel("Time (seconds)")
plt.ylabel("Sensor value (mV)")
plt.title("Measured vs Smooth Sensor Data Over Time")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("./plot2.png")  # save the plot


