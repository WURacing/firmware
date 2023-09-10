import * as React from 'react';
import { useEffect, useState } from 'react';
import Table from '@material-ui/core/Table';
import TableBody from '@material-ui/core/TableBody';
import TableCell from '@material-ui/core/TableCell';
import TableHead from '@material-ui/core/TableHead';
import TableRow from '@material-ui/core/TableRow';
import Typography from '@material-ui/core/Typography';
import KeyboardArrowDownIcon from '@material-ui/icons/KeyboardArrowDown';
import KeyboardArrowUpIcon from '@material-ui/icons/KeyboardArrowUp';
import Box from '@material-ui/core/Box';
import { makeStyles } from '@material-ui/core/styles';
import Collapse from '@material-ui/core/Collapse';
import IconButton from '@material-ui/core/IconButton';
import GetAppIcon from '@material-ui/icons/GetApp';
import Button from '@material-ui/core/Button';


function tConvert(time) {
    // Check correct time format and split into components
    time = time.toString ().match (/^([01]\d|2[0-3])(:)([0-5]\d)(:[0-5]\d)?$/) || [time];
  
    if (time.length > 1) { // If time format correct
      time = time.slice (1);  // Remove full string match value
      time[5] = +time[0] < 12 ? ' AM' : ' PM'; // Set AM/PM
      time[0] = +time[0] % 12 || 12; // Adjust hours
    }
    return time.join (''); // return adjusted time or original string
}


//Styling for the rows, names follow matching components unless otherwise specified 
const useRowStyles =makeStyles(
    {root: {
        '& > *': {
          borderBottom: 'unset',
        },
      },
    }
);

//styling options for the dates
const dateOptions = {
    weekday: 'short',
    year: 'numeric', 
    month: 'short', 
    day: '2-digit'
}


//Each run has a set of intervals this function breaks that interval down to the important data for display 
function createIntervalRow(interval, testingDate, rootApiPath){
    var startTime = interval.start.slice(0,8) 
    var start = tConvert(startTime)
    var endTime = interval.end.slice(0, 8)
    var end = tConvert(endTime)
    var apiLink = rootApiPath + '/' + testingDate + "/" + interval.start + '/' + interval.end + '/data.csv';
    return({
        intervalStart: start, 
        intervalEnd: end, 
        intervalApiLink: apiLink, 
        intervalDescription: interval.description
    })
}

//A fragment of the run table that takes in the data for the current run and creates collapsing table row for all the run data
export default function RunTableRow(props){
    const [isOpen, setIsOpen] = useState(false);
    var intervalRows = [];
    const classes = useRowStyles;
    var date = new Date(props.run.date).toLocaleDateString(dateOptions)
    var startTime = props.run.start.slice(0,8) 
    var start = tConvert(startTime)
    var endTime = props.run.end.slice(0, 8)
    var end = tConvert(endTime)
    var apiLink = props.rootApiPath + '/' + props.run.date + '/' + props.run.start + '/' + props.run.end +'/data.csv'
    props.run.intervals.forEach(interval => {
        intervalRows.push(createIntervalRow(interval, props.run.date, props.rootApiPath))
    });
    return(
        <React.Fragment>
            <TableRow className={classes.root}>
                <TableCell>
                    <IconButton aria-label="expand row" size="small" 
                    onClick={() => {setIsOpen(!isOpen)}}>
                        {isOpen ? <KeyboardArrowUpIcon/> : <KeyboardArrowDownIcon />}
                   </IconButton>
                </TableCell>
                <TableCell component="th" scope="row">{date}</TableCell>
                <TableCell>{props.run.location}</TableCell>
                <TableCell>{start}</TableCell>
                <TableCell>{end}</TableCell>
                <TableCell>
                {/* Button is using href to download files as of now. This may need to change in the future */}
                <Button
                    variant="contained"
                    color="primary"
                    size="small"
                    className={classes.button}
                    startIcon={<GetAppIcon />}
                    href={apiLink}
                >
                    Save
                </Button>
                </TableCell>
            </TableRow>
            <TableRow>
                <TableCell style={{ paddingBottom: 0, paddingTop: 0 }} colSpan={6}>
                    <Collapse in={isOpen} timeout='auto' unmountOnExit>
                        <Box margin={1}>
                        <Typography variant="h6" gutterBottom component="div">
                            Intervals
                        </Typography>
                        <Table size ='small'>
                            <TableHead>
                                <TableRow>
                                    <TableCell>Description</TableCell>
                                    <TableCell>Start Time</TableCell>
                                    <TableCell>End Time</TableCell>
                                    <TableCell>Download</TableCell>
                                </TableRow>  
                            </TableHead>
                            <TableBody>
                                {intervalRows.map((interval) =>(
                                <TableRow>
                                    <TableCell component="th" scope="row">{interval.intervalDescription}</TableCell>
                                    <TableCell>{interval.intervalStart}</TableCell>
                                    <TableCell>{interval.intervalEnd}</TableCell>
                                    <TableCell>
                                        <Button
                                            variant="contained"
                                            color="primary"
                                            size="small"
                                            className={classes.button}
                                            startIcon={<GetAppIcon />}
                                            href ={interval.intervalApiLink}
                                        >
                                            Save
                                        </Button>
                                        
                                    </TableCell>
                                </TableRow>
                                 ))}
                            </TableBody>
                        </Table> 
                        </Box>
                    </Collapse>
                </TableCell>
            </TableRow>
        </React.Fragment>
    )
}