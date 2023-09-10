import * as React from 'react';
import { useEffect, useState } from 'react';
import Table from '@material-ui/core/Table';
import TableBody from '@material-ui/core/TableBody';
import TableCell from '@material-ui/core/TableCell';
import TableContainer from '@material-ui/core/TableContainer';
import TableHead from '@material-ui/core/TableHead';
import TableRow from '@material-ui/core/TableRow';
import Paper from '@material-ui/core/Paper';

import RunTableRow from './RunTableRow';


//makes api call to get all current runs and then breaks that down into tablecells using RunTableRow
// This is more of a wrapper component for the fragments created by RunTableRow
export default function RunTable (props){
    const [isLoaded, setIsLoaded] = useState(null);
    const [runData, setRunData] = useState(null);
    const [error, setError] = useState(null);
    useEffect(() =>{
        if(!isLoaded){
            fetch(props.rootApiPath)
                    .then(res => res.json())
                    .then(
                        (result) =>{
                            setIsLoaded(true);
                            setRunData(result);
                        },
                    (error) => {
                        setIsLoaded(true);
                        setError(error);
                    })
        }
    })
    if(error){
        return <div>Error</div>
    }
    else if(!isLoaded || runData == null){
        return <div>Did not load</div>
    }
    else{
        console.log(runData)
        return (
            <TableContainer component={Paper}>
                <Table aria-label="collapsible table">
                <TableHead>
                    <TableRow>
                    <TableCell />
                    <TableCell>Date</TableCell>
                    <TableCell>Location</TableCell>
                    <TableCell>Start Time</TableCell>
                    <TableCell>End Time</TableCell>
                    <TableCell>Download</TableCell>
                    </TableRow>
                </TableHead>
                <TableBody>
                    {runData.map((row) => (
                        <RunTableRow run={row} rootApiPath={props.rootApiPath}/>
                    ))}
                </TableBody>
                </Table>
            </TableContainer>
            );
    }
}