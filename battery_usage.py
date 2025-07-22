
current_drawn =  0.1 #mA
daily_current = current_drawn * 24 # mAh/day

battery_size = 400 #mAh

lifetime_battery = battery_size / daily_current

print(lifetime_battery)

