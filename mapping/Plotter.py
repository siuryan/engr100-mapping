import matplotlib.pyplot as plt

class Plotter:

    def __init__(self):
        self.points = []

    def add_point(self, x, y):
        self.points.append((x, y))

    def plot(self):
        points_x = [point[0] for point in self.points]
        points_y = [point[1] for point in self.points]
        plt.plot(points_x, points_y, 'ro')
        plt.show()
