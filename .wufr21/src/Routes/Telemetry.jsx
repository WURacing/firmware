import './App.css';
import Dashboard from '../Components/Dashboard/Dashboard';


const rootApiPath = "https://data.washuracing.com/api/v2/testing"
const Telemetry = () => {
  return (
    <Dashboard rootApiPath={rootApiPath} />
  );
}

export default Telemetry;
