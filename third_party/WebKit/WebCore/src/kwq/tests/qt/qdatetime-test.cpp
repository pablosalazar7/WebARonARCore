#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include <qdatetime.h>

int main() {

    //QDate d0;
    QDate d1 = QDate(1999, 12, 31);
    QDate d2 = QDate(2000, 12, 31);
    
    //QTime t0;
    QTime t1 = QTime(23, 59, 59, 999);
    QTime t2 = QTime(23, 59, 59, 999);
    
    QDateTime dt0;    
    QDateTime dt1 = QDateTime(d1);    
    QDateTime dt2 = QDateTime(d1, t1);    
    QDateTime dt3 = QDateTime(d2, t2);
    
    cout << "dt1: " << dt1 << endl;
    cout << "dt2: " << dt2 << endl;
    cout << "dt3: " << dt2 << endl;
    
    dt0 = dt1;
    cout << "dt0 = dt1: " << dt0 << endl;
    
    cout << "dt2 secsTo dt3: " << dt2.secsTo(dt3) << endl;
    
    cout << "dt2 time: " << dt2.time();
    
    //cout << "string: " << dt0.toString() << endl; 

    //d0 = d1.addDays(1);
    //dt0.setDate(d0);
    //cout << "setDate: " << dt0 << endl;

    //t0 = t1.addSecs(1);
    //dt0.setTime(t0);
    //cout << "setTime: " << dt0 << endl;

    //dt0.setTime_t(1000000000);
    //cout << "setTime_t: " << dt0 << endl;

    return 0;
}
