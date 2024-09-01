import csv
import matplotlib.pyplot as plt
import numpy as np

FILE_NAME = "../raw/kip2-mxc1.csv"


# Data structures to store validate csv data
x_data = []
x_set = set()
y_set = set()
data_corelation = {}

# Parse the csv data, no repetements
with open(FILE_NAME) as file:
    csv_reader = csv.reader(file, delimiter=',')
    for i, row in enumerate(csv_reader):
        if i == 0:
            continue

        x = row[2]
        y = row[3]

        if x == "" or y == "":
            continue

        x = float(x)
        y = float(y)

        if x not in x_set and y not in y_set:
            if x < 404 and y < 1800:
                continue

            x_data.append(x)
            x_set.add(x)
            y_set.add(y)
            data_corelation[x] = y

# Sort the data to create a good corelation and 
# get the eqivalend y value
x_data.sort()
y_data = []
for kip2 in x_data:
    y_data.append(data_corelation[kip2])

# Calculate a 3th degree polynomial fit
coefficients = np.polyfit(x_data, y_data, 3)
a, b, c, d = coefficients
fit_function = lambda i: (a * i ** 3 + b * i ** 2 + c * i + d)

# Print the coefficients
print("y=ax³+bx²+cx+d")
print(f"a = {a}")
print(f"b = {b}")
print(f"c = {c}")
print(f"d = {d}")

# Fit all the data
x_fit = np.linspace(x_data[0], x_data[-1], len(x_data) * 2)
y_fit = [fit_function(x_element) for x_element in x_fit]

# Plot the final results
plt.scatter(x_data, y_data, c='red', s=1)
plt.plot(x_fit, y_fit)
plt.show()
