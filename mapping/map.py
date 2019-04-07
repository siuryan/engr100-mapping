from Plotter import Plotter
from LogReader import LogReader
from Localizer import Localizer

def main():
    plotter = Plotter()
    log_reader = LogReader()

    log_reader.read_logs('mapping_data.csv', False)
    local = Localizer(log_reader.get_logs(), 2.5, 0.15)

    x = local.get_positions_x()
    y = local.get_positions_y()
    walls = local.get_walls()
    #print (x)
    #print (y)
    #print (walls)

    for i in range(len(x)):
        walls_at_point = [ wall for wall in walls if wall[2] == i ]
        plotter.add_point(x[i], y[i], walls_at_point, plotter.FLIGHT_LABEL)

    for i in range(len(walls)):
        plotter.add_point(walls[i][0], walls[i][1], i, plotter.WALLS_LABEL)

    plotter.plot()

if __name__ == "__main__":
    main()
