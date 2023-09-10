import './App.css';
import RunTable from '../Components/RunTable';
import NavBar from '../Components/NavBar';
import { Box } from '@mui/system';


const rootApiPath = "https://data.washuracing.com/api/v2/testing"
function RunData() {
  return (
    <Box className="page">
      <NavBar></NavBar>
      <Box style={{ marginTop: 80, marginLeft: 80 }} className="runData">
        <RunTable rootApiPath={rootApiPath} />
      </Box>
    </Box>
  );
}

export default RunData;
