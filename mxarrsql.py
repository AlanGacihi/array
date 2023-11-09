from sys import version_info as _swig_python_version_info
if _swig_python_version_info < (2, 7, 0):
    raise RuntimeError("Python 2.7 or later required")

# Import the low-level C/C++ module
if __package__ or "." in __name__:
    from . import _mxarr
else:
    import _mxarr

import sqlite3

class Database:
    def __init__(self, reset=False):
        """
        Initialize the Database object.

        Parameters:
            - reset (bool): If True, delete the existing database file and create a new one.
        """
        if reset:
            # Delete the database file if it exists to start fresh
            try:
                self.conn.close()
            except AttributeError:
                pass  # Ignore if there's no existing connection
            finally:
                import os
                if os.path.exists("mxarr.db"):
                    os.remove("mxarr.db")

        self.conn = sqlite3.connect("mxarr.db")

    def create_tables(self):
        """
        Create database tables for Arrays, DoubleData, and IntegerData if they don't exist.
        """
        cursor = self.conn.cursor()
        cursor.execute('''CREATE TABLE IF NOT EXISTS Arrays (
            NAME VARCHAR(64) PRIMARY KEY NOT NULL,
            DIMNO INTEGER NOT NULL,
            DATATYPE INTEGER NOT NULL,
            DIMS0 INTEGER NOT NULL,
            DIMS1 INTEGER NOT NULL,
            ELNO INTEGER NOT NULL
        )''')

        cursor.execute('''CREATE TABLE IF NOT EXISTS DoubleData (
            ARRAYNAME VARCHAR(64) NOT NULL,
            I INTEGER NOT NULL,
            J INTEGER NOT NULL,
            VALUE FLOAT NOT NULL,
            PRIMARY KEY (ARRAYNAME, I, J),
            FOREIGN KEY (ARRAYNAME) REFERENCES Arrays(NAME)
        )''')

        cursor.execute('''CREATE TABLE IF NOT EXISTS IntegerData (
            ARRAYNAME VARCHAR(64) NOT NULL,
            I INTEGER NOT NULL,
            J INTEGER NOT NULL,
            VALUE INTEGER NOT NULL,
            PRIMARY KEY (ARRAYNAME, I, J),
            FOREIGN KEY (ARRAYNAME) REFERENCES Arrays(NAME)
        )''')

        self.conn.commit()

    def __setitem__(self, table, values):
        """
        Insert values into the specified table.

        Parameters:
            - table (str): Name of the table where values will be inserted.
            - values (tuple): Values to be inserted into the table.
        """
        cursor = self.conn.cursor()
        cursor.execute(f"INSERT INTO {table} VALUES (?, ?, ?)", values)
        self.conn.commit()

    def store_arr(self, arrname, arr):
        """
        Store a two-dimensional array in the database.

        Parameters:
            - arrname (str): Name of the array.
            - arr (numpy.ndarray): Two-dimensional array to be stored in the database.
        """
        if arr.ndim != 2:
            raise ValueError("Array must be two-dimensional.")
        cursor = self.conn.cursor()
        cursor.execute("INSERT INTO Arrays VALUES (?, ?, ?, ?, ?, ?)",
                       (arrname, arr.type, arr.shape[0], arr.shape[1], arr.size))
        self.conn.commit()

        if arr.type == _mxarr.DOUBLE_TYPE:
            data_table = "DoubleData"
        else:
            data_table = "IntegerData"

        for i in range(arr.shape[0]):
            for j in range(arr.shape[1]):
                cursor.execute(f"INSERT INTO {data_table} VALUES (?, ?, ?, ?)",
                               (arrname, i, j, arr[i, j]))

        self.conn.commit()

    def retrieve_arr(self, arrname):
        """
        Retrieve a stored array from the database.

        Parameters:
            - arrname (str): Name of the array to retrieve.

        Returns:
            - _mxarr.Array: The retrieved array.
        """
        cursor = self.conn.cursor()
        cursor.execute("SELECT * FROM Arrays WHERE NAME=?", (arrname,))
        row = cursor.fetchone()

        if row:
            dim0, dim1 = row[3], row[4]
            arr_type = row[2]
            data_table = "DoubleData" if arr_type == _mxarr.DOUBLE_TYPE else "IntegerData"

            arr = _mxarr.Array(dim0 * dim1, arr_type)
            arr.inflate(dim0)
            cursor.execute(f"SELECT * FROM {data_table} WHERE ARRAYNAME=?", (arrname,))
            data = cursor.fetchall()

            for row in data:
                i, j, value = row[1], row[2], row[3]
                arr[i, j] = value

            return arr

if __name__ == "__main__":
    db = Database(reset=True)
    db.create_tables()

    a1 = _mxarr.Array(9, _mxarr.DOUBLE_TYPE)
    a1.inflate(3)
    a2 = _mxarr.Array(9, _mxarr.UCHAR_TYPE)
    a2.inflate(3)

    a1[0, 0] = 1.0
    a1[0, 1] = 2.0
    a1[0, 2] = 3.0
    a1[1, 0] = 13.0
    a1[1, 1] = 17.0
    a1[1, 2] = 19.0
    a1[2, 0] = 37.0
    a1[2, 1] = 41.0
    a1[2, 2] = 43.0

    a2[0, 0] = 5
    a2[0, 1] = 23
    a2[0, 2] = 47
    a2[1, 0] = 7
    a2[1, 1] = 29
    a2[1, 2] = 53
    a2[2, 0] = 11
    a2[2, 1] = 31
    a2[2, 2] = 59

    db.store_arr("a1", a1)
    del a1
    db.store_arr("a2", a2)
    del a2

    db.conn.close()
    # reopen database connection
    db = Database()
    a1 = db.retrieve_arr("a1")
    a2 = db.retrieve_arr("a2")

    print("a1 =")
    for i in range(3):
        for j in range(3):
            print("%6.1f " % a1[i, j], end="")
        print()
    print()
    print()
    print("a2 =")
    for i in range(3):
        for j in range(3):
            print("%6d " % a2[i, j], end="")
        print()
