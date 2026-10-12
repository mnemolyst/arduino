from functools import reduce

bounds = [[46, 66], [145, 163], [230, 251], [310, 328], [393, 411], [452, 471]]
print(', '.join([
    str(i) for j in bounds for i in range(j[0], j[1] + 1)
]))
print(reduce(lambda acc, i: acc + i[1] - i[0] + 1, bounds, 0))
