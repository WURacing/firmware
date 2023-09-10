import * as React from 'react';
import { useHistory } from 'react-router-dom';
import { styled, useTheme } from '@mui/material/styles';
import Box from '@mui/material/Box';
import MuiDrawer from '@mui/material/Drawer';
import MuiAppBar from '@mui/material/AppBar';
import Toolbar from '@mui/material/Toolbar';
import List from '@mui/material/List';
import CssBaseline from '@mui/material/CssBaseline';
import Typography from '@mui/material/Typography';
import IconButton from '@mui/material/IconButton';
import { Menu } from '@material-ui/icons';
import { ChevronLeft, ChevronRight } from '@material-ui/icons';
import ListItem from '@mui/material/ListItem';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemText from '@mui/material/ListItemText';
import { SvgIcon } from '@mui/material';
import { ReactComponent as Logo } from './images/logo.svg'
import { Storage, Dashboard, CloudDownload, Restore, Info } from '@material-ui/icons';


const drawerWidth = 240;

const openedMixin = (theme) => ({
  color: 'white',
  background: '#0F0F0F',
  width: drawerWidth,
  transition: theme.transitions.create('width', {
    easing: theme.transitions.easing.sharp,
    duration: theme.transitions.duration.enteringScreen,
  }),
  overflowX: 'hidden',
});

const closedMixin = (theme) => ({
  color: 'white',
  background: '#0F0F0F',
  transition: theme.transitions.create('width', {
    easing: theme.transitions.easing.sharp,
    duration: theme.transitions.duration.leavingScreen,
  }),
  overflowX: 'hidden',
  width: `calc(${theme.spacing(7)} + 1px)`,
  [theme.breakpoints.up('sm')]: {
    width: `calc(${theme.spacing(9)} + 1px)`,
  },
});

const DrawerHeader = styled('div')(({ theme }) => ({
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'flex-end',
  padding: theme.spacing(0, 1),
  // necessary for content to be below app bar
  ...theme.mixins.toolbar,
}));

const AppBar = styled(MuiAppBar, {
  shouldForwardProp: (prop) => prop !== 'open',
})(({ theme, open }) => ({
  background: 'linear-gradient(90deg, rgba(0,0,0,1) 0%, rgba(182,7,7,1) 100%, rgba(0,212,255,1) 100%);',
  zIndex: theme.zIndex.drawer + 1,
  transition: theme.transitions.create(['width', 'margin'], {
    easing: theme.transitions.easing.sharp,
    duration: theme.transitions.duration.leavingScreen,
  }),
  ...(open && {
    marginLeft: drawerWidth,
    width: `calc(100% - ${drawerWidth}px)`,
    transition: theme.transitions.create(['width', 'margin'], {
      easing: theme.transitions.easing.sharp,
      duration: theme.transitions.duration.enteringScreen,
    }),
  }),
}));

const Drawer = styled(MuiDrawer, { shouldForwardProp: (prop) => prop !== 'open' })(
  ({ theme, open }) => ({
    width: drawerWidth,
    flexShrink: 0,
    whiteSpace: 'nowrap',
    boxSizing: 'border-box',
    ...(open && {
      ...openedMixin(theme),
      '& .MuiDrawer-paper': openedMixin(theme),
    }),
    ...(!open && {
      ...closedMixin(theme),
      '& .MuiDrawer-paper': closedMixin(theme),
    }),
  }),
);

const routeTexts = {
  "Run Data": 'rundata',
  "Live Telemetry": 'liveTelemetry',
  'Recorded Telemetry': 'recordedTelemetry',
  'About': 'about'
}


export default function NavBar(props) {
  const history = useHistory();
  const theme = useTheme();
  const [open, setOpen] = React.useState(false);
  const icons = [<Dashboard style={{ fill: 'white' }} />, <Storage style={{ fill: 'white' }} />, <CloudDownload style={{ fill: 'white' }} />
    , <Restore style={{ fill: 'white' }} />, <Info style={{ fill: 'white' }} />];
  const handleDrawerOpen = () => {
    setOpen(true);
  };

  const handleDrawerClose = () => {
    setOpen(false);
  };

  return (
    <Box sx={{ display: 'flex' }}>
      <CssBaseline />
      <AppBar position="fixed" open={open}>
        <Toolbar>
          <IconButton
            color="inherit"
            aria-label="open drawer"
            onClick={handleDrawerOpen}
            edge="start"
            sx={{
              marginRight: '36px',
              ...(open && { display: 'none' }),
            }}
          >
            <Menu />
          </IconButton>
          <SvgIcon component={Logo} style={{ paddingTop: 15, paddingRight: 10, fontSize: 60, scale: '125%' }} viewBox="0 0 600 476.6"></SvgIcon>
          <Typography style={{ paddingLeft: 15 }} variant="h6" noWrap component="div">
            Data Viewer
          </Typography>
        </Toolbar>
      </AppBar>
      <Drawer variant="permanent" open={open}>
        <DrawerHeader>
          <IconButton onClick={handleDrawerClose}>
            {theme.direction === 'rtl' ? <ChevronRight style={{ fill: 'white' }} /> : <ChevronLeft style={{ fill: 'white' }} />}
          </IconButton>
        </DrawerHeader>
        <List>
          {['Dashboard', 'Run Data', 'Live Telemetry', 'Recorded Telemetry', 'About'].map((text, index) => (
            <ListItem button key={text} onClick={() => {
              history.push("/" + routeTexts[text]);
            }}>
              <ListItemIcon>
                {icons[index]}
                {/* {index % 2 === 0 ? <Inbox style={{fill:'white'}} /> : <Mail style={{fill:'white'}}/>} */}
              </ListItemIcon>
              <ListItemText primary={text} />
            </ListItem>
          ))}
        </List>
      </Drawer>
    </Box>
  );
}
