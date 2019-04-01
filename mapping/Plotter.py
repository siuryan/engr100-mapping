import matplotlib.pyplot as plt

class Plotter:

    def __init__(self):
        self.COLORS = ['red', 'black']

        self.points = {}

        self._plot_settings()

    def add_point(self, x, y, label=0):
        if label not in self.points:
            self.points[label] = []
        self.points[label].append((x, y))

    def _plot_settings(self):
        plt.axis('equal')

    def plot(self):
        for i, label in enumerate(self.points.keys()):
            points_x = [point[0] for point in self.points[label]]
            points_y = [point[1] for point in self.points[label]]

            # TODO: this is temp, rewrite in a better way
            if label == 0:
                plt.plot(
                    points_x, points_y, 'o',
                    color=self.COLORS[i % len(self.COLORS)])
            else:
                plt.plot(
                    points_x, points_y,
                    color=self.COLORS[i % len(self.COLORS)])

        plt.show()
