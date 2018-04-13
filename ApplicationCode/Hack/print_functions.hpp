//
// Created by bellout on 4/6/18.
//

// ---------------------------------------------------------------
#ifndef RESINSIGHT_PRINT_FUNCTIONS_HPP
#define RESINSIGHT_PRINT_FUNCTIONS_HPP

// ---------------------------------------------------------------
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <cvfVector3.h>

// ---------------------------------------------------------------
using std::remove;
using std::string;
using std::ostringstream;
using std::fstream;

// ---------------------------------------------------------------
namespace RIHack {

// RI print functions

// ---------------------------------------------------------------
/*!
 * \brief Get time stamp
 */
inline string get_time_stamp() {

  // -------------------------------------------------------------
  time_t ltime;
  struct tm *Tm;
  char ts_char [50];
  string ts_str;

  // -------------------------------------------------------------
  ltime = time(nullptr); /* get current cal time */
  sprintf(ts_char, "%s", asctime( localtime(&ltime) ) );
  ts_str = string(ts_char);

  // -------------------------------------------------------------
  // Remove newline character
  ts_str.erase(remove(ts_str.begin(), ts_str.end(), '\n'),
               ts_str.end());

  return ts_str;
}

// ---------------------------------------------------------------
/*!
 * \brief Set stream format
 */
inline void set_str_frmt(ostringstream &dbg_msg) {
  // ostringstream dbg_msg;
//  dbg_msg.precision(3);
//  dbg_msg.setf(ios::fixed, ios::floatfield);
//  dbg_msg.setf(ios::adjustfield, ios::right);
  // return dbg_msg;
};

// ---------------------------------------------------------------
/*!
 * \brief Prints debug messages template
 * @param debug_msg
 * @param dbg_locrig
 * Use:
  print_grd_dbg(true, true, dbg_loc, dbg_msg)
 *
 */
inline void print_dbg_template(bool dbg_mode, bool append,
                               const string file_name,
                               string dbg_func,
                               string dbg_file,
                               string dbg_msgs) {

  // -------------------------------------------------------------
  fstream fs;
  if (dbg_mode) {
    if (append) {
      fs.open(file_name, std::fstream::out | std::fstream::app);
    } else {
      fs.open(file_name, std::fstream::out | std::fstream::trunc);
    }

    // -----------------------------------------------------------
    // dbg_func
    string dbg_func_fxd;
    string ts = get_time_stamp();
    dbg_func_fxd = "[" + ts + "] (" + dbg_func + ") => ";

    // -----------------------------------------------------------
    // dbg_file
    QStringList qt_str;
    qt_str << QString::fromStdString(dbg_file).split("/");
    string dbg_file_fxd = "[" + qt_str.last().toStdString() + "]";

    // -----------------------------------------------------------
    // write: dbg_func
    if (!dbg_func.empty()) {
      fs.write(dbg_func_fxd.c_str(), dbg_func_fxd.size());
    }

    // -----------------------------------------------------------
    // write: dbg_file
    if (!dbg_file.empty()) {
      fs.write(dbg_file_fxd.c_str(), dbg_file_fxd.size());
    }

    // -----------------------------------------------------------
    // write: dbg_msgs
    if (!dbg_msgs.empty()) {
      if (dbg_func.empty() && dbg_file.empty()) {
        dbg_msgs = "-> " + dbg_msgs;
      } else {
        dbg_msgs = "\n-> " + dbg_msgs;
      }
      fs.write(dbg_msgs.c_str(), dbg_msgs.size());
    }

    // -----------------------------------------------------------
    // write: end line
    string endline = "\n";
    fs.write(endline.c_str(), endline.size());

    // -----------------------------------------------------------
    fs.close();
  }
};

// ---------------------------------------------------------------
/*!
 * \brief Prints debug messages for optimizer system
 * @param debug_msg
 * @param dbg_loc
 * Use:
  print_opt_dbg(true, true, dbg_loc, dbg_msg)
 *
 */
inline void print_ri_hck(
    string dbg_func, string dbg_file, string dbg_msgs = "",
    bool dbg_mode = true, bool append = true) {

  print_dbg_template(dbg_mode, append, "ri.hck",
                     dbg_func, dbg_file, dbg_msgs);
};

// ---------------------------------------------------------------
inline void print_ri_hck_vec(
    string dbg_func, string dbg_file, string dbg_msgs = "",
    cvf::Vec3d dbg_vec = cvf::Vec3d::ZERO,
    bool dbg_mode = true, bool append = true) {

  dbg_msgs = dbg_msgs
      + "[ x = " + std::to_string(dbg_vec.x())
      + ", y = " + std::to_string(dbg_vec.y())
      + ", z = " + std::to_string(dbg_vec.z()) + " ]";

  print_dbg_template(dbg_mode, append, "ri.hck",
                     dbg_func, dbg_file, dbg_msgs);
};



// ---------------------------------------------------------------
/*!
 * \brief Prints debug messages for gradient assembly system
 * @param debug_msg
 * @param dbg_loc
 * Use:
  print_grd_dbg(true, true, dbg_loc, dbg_msg)
 *
 */
//inline void print_grd_dbg(bool dbg_mode, bool append,
//                          string dbg_loc, string dbg_msg) {
//  print_dbg_template(dbg_mode, append, "grd.dbg", dbg_loc, dbg_msg);
//};

}

#endif //RESINSIGHT_PRINT_FUNCTIONS_HPP
