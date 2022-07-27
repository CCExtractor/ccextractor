
pub fn millis_to_time(milli: i64, hours: &mut u8, minutes: &mut u8, seconds: &mut u8, ms: &mut u16) {
    *ms = (milli % 1000) as u16;
    let mut left_time:i64= (milli - *ms as i64) / 1000; // remainder in seconds
    *seconds = (left_time % 60) as u8;
    left_time = (left_time - *seconds as i64) / 60;
    *minutes = (left_time % 60) as u8;
    left_time = (left_time - *minutes as i64) / 60;
    *hours = left_time as u8;
}

pub fn print_mstime_buff(mut mstime:i64) -> String{
    let signoffset = if mstime < 0 {1} else {0};

    if mstime < 0 {
        mstime = -mstime;
    }
    let hh = mstime / 1000 / 60 / 60;
    let mm = mstime / 1000 / 60 - 60 * hh;
    let ss = mstime / 1000 - 60 * (mm + 60 * hh);
    let ms = mstime - 1000 * (ss + 60 * (mm + 60 * hh));

    format!("{:02}:{:02}:{:02}:{:03}",hh,mm,ss,ms)
}

