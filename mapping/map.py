from Plotter import Plotter
from LogReader import LogReader
from Localizer import Localizer

def main():
    plotter = Plotter()
    log_reader = LogReader()

    log_reader.read_logs('sample-log.txt', True)
    local = Localizer(log_reader.get_logs())

    x = local.get_positions_x()
    y = local.get_positions_y()
    print (x)
    print (y)
    print (local.get_walls())

    for i in range(len(x)):
        plotter.add_point(x[i], y[i])

    plotter.plot()

if __name__ == "__main__":
    main()
