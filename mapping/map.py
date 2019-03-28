from Plotter import Plotter
from LogReader import LogReader
from Localizer import Localizer

def main():
    plotter = Plotter()
    log_reader = LogReader()

    log_reader.read_logs('sample-log.txt', True)
    local = Localizer(log_reader.get_logs(), 0.5)

    x = local.get_positions_x()
    y = local.get_positions_y()
    walls = local.get_walls()
    print (x)
    print (y)
    print (walls)

    for i in range(len(x)):
        plotter.add_point(x[i], y[i])
        # plotter.add_point(walls[i][0], walls[i][1])

    plotter.plot()

if __name__ == "__main__":
    main()
