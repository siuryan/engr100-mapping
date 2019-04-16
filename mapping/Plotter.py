import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
import numpy as np

class Plotter:

    def __init__(self):
        self.FLIGHT_LABEL = 0
        self.WALLS_LABEL = 1

        self.points = {}

        self.dynamic_points = []

        self._plot_settings()

    def add_point(self, x, y, data, label=0):
        if label not in self.points:
            self.points[label] = []
        self.points[label].append((x, y, data))

    def _get_point_data(self, x, y):
        for point in self.points[self.FLIGHT_LABEL]:
            if point[0] == x and point[1] == y:
                return [ (wall[0], wall[1]) for wall in point[2] ]

        return None

    def _plot_settings(self):
        plt.axis('equal')
        plt.xlabel('x (m)')
        plt.ylabel('y (m)')
        fig = plt.gcf()

        def pickpoint(event):
            if isinstance(event.artist, Line2D):
                thisline = event.artist
                xdata = thisline.get_xdata()
                ydata = thisline.get_ydata()
                ind = event.ind
                x = np.take(xdata, ind)[0]
                y = np.take(ydata, ind)[0]
                data = self._get_point_data(x, y)
                print ('X='+str(x))
                print ('Y='+str(y))
                print ('walls='+str(data))

                lines = plt.gca().lines

                for datum in self.dynamic_points:
                    lines.pop()

                for datum in data:
                    plt.plot(datum[0], datum[1], 'o', color='blue', markersize=5)

                self.dynamic_points = data

                fig.canvas.draw_idle()

        fig.canvas.mpl_connect('pick_event', pickpoint)

    def plot(self):
        for label in self.points:
            points_x = [point[0] for point in self.points[label]]
            points_y = [point[1] for point in self.points[label]]

            if label == self.FLIGHT_LABEL:
                plt.plot(
                    points_x, points_y, 'o',
                    color='red', label=label, markersize=2, picker=5)
            else:
                plt.plot(
                    points_x, points_y, 'o',
                    color='black', label=label, markersize=5)

        plt.show()
